#!/usr/bin/make -f
initial_variables :=$(.VARIABLES)

include ./common.mk

# temporary prefix for copying files in install rule
# if set then must end with '/'
DESTDIR :=

VERSION := 0.3.3
version_numbers :=$(subst ., ,$(VERSION))
version_major :=$(word 1,$(version_numbers))

# Common prefix for installation directories.
prefix :=/usr/local
libdir =$(prefix)/lib
includedir =$(prefix)/include

INSTALL :=install
INSTALL_LIB =$(INSTALL) -m 644
INSTALL_INCLUDE =$(INSTALL) -m 644

# Test for presence of boost_system. Currently, only actually needed part is Asio
# that is used in extended tests and this seems to be reasonably efficient way to detect it.
BOOST_LIB_PATHS = $(shell /sbin/ldconfig -p | grep 'libboost_system')
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
$(call collapse-slashes,mkdir -p $1)

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
	mkdir -p $@


test-basic:
	+$(MAKE) -C ./tests/ test
test-extended:
ifeq ($(HAVE_BOOST_SYSTEM),1)
	+$(MAKE) -C ./tests-extended/ test
else
	$(error Extended tests skipped - Boost (libboost_system) is required and wasn't detected)
endif

test: test-basic test-extended


libsuperiormysqlpp.pc: libsuperiormysqlpp.pc.in makefile
	sed \
         -e 's,@VERSION@,$(VERSION),' \
         -e 's,@PREFIX@,$(prefix),' \
         libsuperiormysqlpp.pc.in > $@


install: $(DESTDIR)$(libdir)/.
install: $(DESTDIR)$(includedir)/.
install: libsuperiormysqlpp.pc
install:
	$(call recursive-install,$(INSTALL_INCLUDE),./include,*.hpp,$(DESTDIR)$(includedir)/)
	$(INSTALL) -d $(DESTDIR)/$(libdir)/pkgconfig
	$(INSTALL) -t $(DESTDIR)/$(libdir)/pkgconfig -m 644 libsuperiormysqlpp.pc



clean:
	find ./ -type f -name "core" -exec $(RM) {} \;
	+$(MAKE) -C ./tests/ clean
	+$(MAKE) -C ./tests-extended/ clean

clean-all: clean packages-clean-all


deb_packages := debian-stretch debian-jessie szn-debian-wheezy
rpm_packages := fedora-22
packages := $(deb_packages) $(rpm_packages)

define deb-package
package-$1-build:
	+cd packages/$1/dpkg-jail/ && dpkg-buildpackage -j$(CONCURRENCY) -B -us -uc

package-$1-build-install-dependencies:
	cd packages/$1/dpkg-jail/ && mk-build-deps -i -r -t 'apt-get -f -y'

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
	cd packages/$1/ && dnf builddep -y *.spec

package-$1-clean:
	$(RM) -R $(abspath ./)/packages/$1/rpmbuild/

package-$1-clean-packages:
	$(RM) packages/$1/*.rpm

package-$1-clean-all: package-$1-clean package-$1-clean-packages

.PHONY: package-$1-build package-$1-build-install-dependencies package-$1-clean package-$1-clean-packages package-$1-clean-all

endef
$(eval $(foreach package,$(rpm_packages),$(call rpm-package,$(package))))


ifneq "$(CONCURRENCY)" ""
docker_run_concurrency :=-e "CONCURRENCY=$(CONCURRENCY)"
endif

# LeakSanitizer (lsan) needs ptrace and by default docker denies it
define any-package
package-$1-dbuild:
	cd packages/$1/ && $(docker_build) --tag=package-$1-dbuild .
	docker run --name $(IMAGE_PREFIX)dbuild-$1 -t -v /var/run/docker.sock:/var/run/docker.sock -v `pwd`:/dbuild/sources $(docker_run_concurrency) --cap-add SYS_PTRACE package-$1-dbuild
	docker rm $(IMAGE_PREFIX)dbuild-$1 2>&1 1>/dev/null

.PHONY: package-$1-dbuild

endef
$(eval $(foreach package,$(packages),$(call any-package,$(package))))


packages-clean: $(foreach package,$(packages),package-$(package)-clean )


packages-clean-all: $(foreach package,$(packages),package-$(package)-clean-all )
	$(RM) libsuperiormysqlpp-*.tar.*

packages-build: $(foreach package,$(packages),package-$(package)-build )

packages-dbuild: $(foreach package,$(packages),package-$(package)-dbuild )


package-tar.gz: clean-all
	tar -czf libsuperiormysqlpp-$(VERSION).tar.gz *

package-tar.xz: clean-all
	tar -cJf libsuperiormysqlpp-$(VERSION).tar.xz *
