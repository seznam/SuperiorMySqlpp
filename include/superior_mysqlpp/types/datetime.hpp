/*
 * Author: Tomas Nozicka
 */

#pragma once

#include <utility>

#include <superior_mysqlpp/types/mysql_time.hpp>
#include <superior_mysqlpp/types/date.hpp>
#include <superior_mysqlpp/types/time.hpp>
#include <superior_mysqlpp/prepared_statements/binding_types.hpp>


namespace SuperiorMySqlpp
{
    class Datetime : public detail::MysqlTime, public detail::DateBase<Datetime>, public detail::TimeBase<Datetime>
    {
        friend class detail::DateBase<Datetime>;
        friend class detail::TimeBase<Datetime>;

    public:
        Datetime() = default;

        Datetime(Year_t year, Month_t month, Day_t day, Hour_t hour=0, Minute_t minute=0, Second_t second=0, SecondFraction_t secondFraction=0)
        {
            setYear(year);
            setMonth(month);
            setDay(day);
            setHour(hour);
            setMinute(minute);
            setSecond(second);
            setSecondFraction(secondFraction);
        }

        Datetime(Date date, Time time, SecondFraction_t secondFraction=0)
        {
            setDate(std::move(date));
            setTime(std::move(time));
            setSecondFraction(secondFraction);
        }

        Datetime(const Datetime&) = default;
        Datetime(Datetime&&) = default;
        Datetime& operator=(const Datetime&) = default;
        Datetime& operator=(Datetime&&) = default;
        ~Datetime() = default;

        auto getSecondFraction() const
        {
            return buffer.second_part;
        }

        template<typename U>
        void setSecondFraction(U&& secondFraction)
        {
            buffer.second_part = std::forward<U>(secondFraction);
        }

        Date getDate() const
        {
            return {getYear(), getMonth(), getDay()};
        }

        template<typename U>
        void setDate(U&& value)
        {
            setYear(std::forward<U>(value).getYear());
            setMonth(std::forward<U>(value).getMonth());
            setDay(std::forward<U>(value).getDay());
        }

        Time getTime() const
        {
            return {getHour(), getMinute(), getSecond()};
        }

        template<typename U>
        void setTime(U&& value)
        {
            setHour(std::forward<U>(value).getHour());
            setMinute(std::forward<U>(value).getMinute());
            setSecond(std::forward<U>(value).getSecond());
        }
    };


    namespace detail
    {
        template<>
        struct CanBindAsParam<BindingTypes::Datetime, Datetime> : std::true_type {};

        template<>
        struct CanBindAsResult<BindingTypes::Datetime, Datetime> : std::true_type {};
    }



    class Timestamp : public Datetime
    {
        using Datetime::Datetime;
    };


    namespace detail
    {
        template<>
        struct CanBindAsParam<BindingTypes::Timestamp, Timestamp> : std::true_type {};

        template<>
        struct CanBindAsResult<BindingTypes::Timestamp, Timestamp> : std::true_type {};
    }



    inline bool operator==(const Datetime& lhs, const Datetime& rhs)
    {
        return lhs.getDate()==rhs.getDate() &&
               lhs.getTime()==rhs.getTime() &&
               lhs.getSecondFraction()==rhs.getSecondFraction();
    }

    inline bool operator!=(const Datetime& lhs, const Datetime& rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const Datetime& lhs, const Datetime& rhs)
    {
        if (lhs.getDate() < rhs.getDate())
        {
            return true;
        }
        else if (lhs.getDate() > rhs.getDate())
        {
            return false;
        }

        if (lhs.getTime() < rhs.getTime())
        {
            return true;
        }
        else if (lhs.getTime() > rhs.getTime())
        {
            return false;
        }

        return lhs.getSecondFraction() < rhs.getSecondFraction();
    }

    inline bool operator<=(const Datetime& lhs, const Datetime& rhs)
    {
        if (lhs.getDate() < rhs.getDate())
        {
            return true;
        }
        else if (lhs.getDate() > rhs.getDate())
        {
            return false;
        }

        if (lhs.getTime() < rhs.getTime())
        {
            return true;
        }
        else if (lhs.getTime() > rhs.getTime())
        {
            return false;
        }

        return lhs.getSecondFraction() <= rhs.getSecondFraction();
    }

    inline bool operator>(const Datetime& lhs, const Datetime& rhs)
    {
        if (lhs.getDate() > rhs.getDate())
        {
            return true;
        }
        else if (lhs.getDate() < rhs.getDate())
        {
            return false;
        }

        if (lhs.getTime() > rhs.getTime())
        {
            return true;
        }
        else if (lhs.getTime() < rhs.getTime())
        {
            return false;
        }

        return lhs.getSecondFraction() > rhs.getSecondFraction();
    }

    inline bool operator>=(const Datetime& lhs, const Datetime& rhs)
    {
        if (lhs.getDate() > rhs.getDate())
        {
            return true;
        }
        else if (lhs.getDate() < rhs.getDate())
        {
            return false;
        }

        if (lhs.getTime() > rhs.getTime())
        {
            return true;
        }
        else if (lhs.getTime() < rhs.getTime())
        {
            return false;
        }

        return lhs.getSecondFraction() >= rhs.getSecondFraction();
    }
}

