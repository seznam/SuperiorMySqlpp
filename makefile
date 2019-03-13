#!/usr/bin/make -f
initial_variables :=$(.VARIABLES)

include ./common.mk

# temporary prefix for copying files in install rule
# if set then must end with '/'
DESTDIR :=

VERSION := 0.4.0
version_numbers :=$(subst ., ,$(VERSION))
version_major :=$(word 1,$(version_numbers))

# Common prefix for installation directories.
prefix :=/usr/local
libdir =$(prefix)/lib
includedir =$(prefix)/include

INSTALL :=install
INSTALL_LIB =$(INSTALL) --mode=644
INSTALL_INCLUDE =$(INSTALL) --mode=644

# Test for presence of boost_system. Currently, only actually needed part is Asio
# that is used in extended tests and this seems to be reasonably efficient way to detect it.
BOOST_LIB_PATHS = $(shell /sbin/ldconfig --print-cache | grep 'libboost_system')
ifneq ($(BOOST_LIB_PATHS),)
	HAVE_BOOST_SYSTEM = 1
else
	HAVE_BOOST_SYSTEM = 0
endif

.PHONY: _print-variables test test-basic test-extended install clean clean-all packages-clean packages-clean-all packages-build packages-dbuild
.PHONY: package-tar.gz package-tar.xz
.NOTPARALLEL: test

collapse-slashes =$(if $(findstring //,$1),$(call collapse-slashes,$(subst //,/,$1)),$(subst //,/,$1))
list-directories =$(filter-out $(call collapse-slashes,$(dir $1/)),$(dir $(wildcard $(call collapse-slashes,$1/*/))))
define make-directory
$(call collapse-slashes,mkdir --parents $1)

endef

define recursive-install-impl
$(call collapse-slashes,$1 $2/$3 $4/$(patsubst $5%,%,$(dir $(call collapse-slashes,$2/$3))))
$(foreach dir,$(call list-directories,$2),$(call make-directory,$4/$(patsubst $5%,%,$(dir)))$(call recursive-install-impl,$1,$(dir),$3,$4,$5))
endef
recursive-install =$(call recursive-install-impl,$1,$2,$3,$4,$2)


all:
	$(error "make without target")


# for debugging purposes
_print-variables:
	$(foreach v,$(filter-out $(initial_variables) initial_variables,$(.VARIABLES)),$(info $(v) = $($(v))))


# pravidlo pro vsechny adresare (makefile nechape v pravidlech "%/:")
%/.:
	mkdir --parents $@


test-basic:
	+$(MAKE) --directory ./tests/ test
test-extended:
ifeq ($(HAVE_BOOST_SYSTEM),1)
	+$(MAKE) --directory ./tests-extended/ test
else
	$(error Extended tests skipped - Boost (libboost_system) is required and was not detected)
endif

test: test-basic test-extended


libsuperiormysqlpp.pc: libsuperiormysqlpp.pc.in makefile
	sed \
         --expression='s,@VERSION@,$(VERSION),' \
         --expression='s,@PREFIX@,$(prefix),' \
         libsuperiormysqlpp.pc.in > $@


install: $(DESTDIR)$(libdir)/.
install: $(DESTDIR)$(includedir)/.
install: libsuperiormysqlpp.pc
install:
	$(call recursive-install,$(INSTALL_INCLUDE),./include,*.hpp,$(DESTDIR)$(includedir)/)
	$(INSTALL) --directory $(DESTDIR)/$(libdir)/pkgconfig
	$(INSTALL) --target-directory=$(DESTDIR)/$(libdir)/pkgconfig --mode=644 libsuperiormysqlpp.pc



clean:
	find ./ -type f -name "core" -exec $(RM) {} \;
	+$(MAKE) --directory ./tests/ clean
	+$(MAKE) --directory ./tests-extended/ clean

clean-all: clean packages-clean-all


deb_packages := debian-stretch debian-jessie
rpm_packages := fedora-22
packages := $(deb_packages) $(rpm_packages)

define deb-package
package-$1-build:
	+cd packages/$1/dpkg-jail/ && dpkg-buildpackage -j$(CONCURRENCY) -B -us -uc

package-$1-build-install-dependencies:
	cd packages/$1/dpkg-jail/ && mk-build-deps --install --remove --tool 'apt-get --fix-broken --assume-yes'

package-$1-clean:
	+$(if $(shell which dh_clean 2>&1 2>/dev/null),cd packages/$1/dpkg-jail/ && dh_clean,)

package-$1-clean-packages:
	$(RM) packages/$1/*.deb packages/$1/*.changes

package-$1-clean-all: package-$1-clean package-$1-clean-packages

.PHONY: package-$1-build package-$1-build-install-dependencies package-$1-dbuild package-$1-clean package-$1-clean-packages package-$1-clean-all

endef
$(eval $(foreach package,$(deb_packages),$(call deb-package,$(package))))


define rpm-package
package-$1-build:
	+cd packages/$1/ && rpmbuild -bb $(strip \
        --define "_build_name_fmt %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm" \
        --define "_topdir $(abspath ./)" \
        --define "_builddir $(abspath ./)" \
        --define "_buildrootdir $(abspath ./)/packages/$1/rpmbuild/" \
        --define "_sourcedir $(abspath ./)" \
        --define "_rpmdir $(abspath ./)/packages/$1/" \
        --define "_smp_mflags -j$(CONCURRENCY)" \
        --define "_srcrpmdir $(abspath ./)/packages/$1/" \
        --define "_specdir $(abspath ./)/packages/$1/" \
	*.spec)

package-$1-build-install-dependencies:
	cd packages/$1/ && dnf builddep --assumeyes *.spec

package-$1-clean:
	$(RM) --recursive $(abspath ./)/packages/$1/rpmbuild/

package-$1-clean-packages:
	$(RM) packages/$1/*.rpm

package-$1-clean-all: package-$1-clean package-$1-clean-packages

.PHONY: package-$1-build package-$1-build-install-dependencies package-$1-clean package-$1-clean-packages package-$1-clean-all

endef
$(eval $(foreach package,$(rpm_packages),$(call rpm-package,$(package))))


ifneq "$(CONCURRENCY)" ""
docker_run_concurrency :=--env="CONCURRENCY=$(CONCURRENCY)"
endif

# LeakSanitizer (lsan) needs ptrace and by default docker denies it
define any-package
package-$1-dbuild:
	cd packages/$1/ && $(docker_build) --tag=package-$1-dbuild .
	docker run --rm --name $(IMAGE_PREFIX)dbuild-$1 --tty $(docker_run_concurrency) --cap-add SYS_PTRACE \
		--volume=/var/run/docker.sock:/var/run/docker.sock --volume=`pwd`:/dbuild/sources \
		package-$1-dbuild

.PHONY: package-$1-dbuild

endef
$(eval $(foreach package,$(packages),$(call any-package,$(package))))


packages-clean: $(foreach package,$(packages),package-$(package)-clean )


packages-clean-all: $(foreach package,$(packages),package-$(package)-clean-all )
	$(RM) libsuperiormysqlpp-*.tar.*

packages-build: $(foreach package,$(packages),package-$(package)-build )

packages-dbuild: $(foreach package,$(packages),package-$(package)-dbuild )


package-tar.gz: clean-all
	tar --create --gzip --file=libsuperiormysqlpp-$(VERSION).tar.gz *

package-tar.xz: clean-all
	tar --create --xz --file=libsuperiormysqlpp-$(VERSION).tar.xz *
