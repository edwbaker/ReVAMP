#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#ifndef _WIN32
#include <sys/time.h>
#endif

#include <vamp-sdk/RealTime.h>
#include <iostream>

_VAMP_SDK_PLUGSPACE_BEGIN(RealTime.cpp)

namespace Vamp {

#define ONE_BILLION 1000000000

RealTime::RealTime(int s, int n) :
    sec(s), nsec(n)
{
    /* Normalize nanoseconds into range. Use explicit +/-= to avoid
       potential sanitizer replacement issues with ++/--; change is
       local to this root-level copy. */
    while (nsec <= -ONE_BILLION && sec > INT_MIN) { nsec += ONE_BILLION; sec -= 1; }
    while (nsec >=  ONE_BILLION && sec < INT_MAX) { nsec -= ONE_BILLION; sec += 1; }
    while (nsec > 0 && sec < 0) { nsec -= ONE_BILLION; sec += 1; }
    while (nsec < 0 && sec > 0) { nsec += ONE_BILLION; sec -= 1; }
}

RealTime
RealTime::fromSeconds(double sec)
{
    if (sec != sec) { // NaN
        std::cerr << "ERROR: NaN/Inf passed to Vamp::RealTime::fromSeconds" << std::endl;
        return RealTime::zeroTime;
    } else if (sec >= 0) {
        return RealTime(int(sec), int((sec - int(sec)) * ONE_BILLION + 0.5));
    } else {
        return -fromSeconds(-sec);
    }
}

RealTime
RealTime::fromMilliseconds(int msec)
{
    return RealTime(msec / 1000, (msec % 1000) * 1000000);
}

#ifndef _WIN32
RealTime
RealTime::fromTimeval(const struct timeval &tv)
{
    return RealTime(int(tv.tv_sec), int(tv.tv_usec * 1000));
}
#endif

std::ostream &operator<<(std::ostream &out, const RealTime &rt)
{
    if (rt < RealTime::zeroTime) {
        out << "-";
    } else {
        out << " ";
    }

    int s = (rt.sec < 0 ? -rt.sec : rt.sec);
    int n = (rt.nsec < 0 ? -rt.nsec : rt.nsec);

    out << s << ".";

    int nn(n);
    if (nn == 0) out << "00000000";
    else while (nn < (ONE_BILLION / 10)) {
        out << "0";
        nn *= 10;
    }
    
    out << n << "R";
    return out;
}

std::string
RealTime::toString() const
{
    std::stringstream out;
    out << *this;

#if (__GNUC__ < 3)
    out << std::ends;
#endif

    std::string s = out.str();

    return s.substr(0, s.length() - 1);
}

std::string
RealTime::toText(bool fixedDp) const
{
    if (*this < RealTime::zeroTime) return "-" + (-*this).toText();

    std::stringstream out;

    if (sec >= 3600) {
        out << (sec / 3600) << ":";
    }

    if (sec >= 60) {
        out << (sec % 3600) / 60 << ":";
    }

    if (sec >= 10) {
        out << ((sec % 60) / 10);
    }

    out << (sec % 10);
    
    int ms = msec();

    if (ms != 0) {
        out << ".";
        out << (ms / 100);
        ms = ms % 100;
        if (ms != 0) {
            out << (ms / 10);
            ms = ms % 10;
        } else if (fixedDp) {
            out << "0";
        }
        if (ms != 0) {
            out << ms;
        } else if (fixedDp) {
            out << "0";
        }
    } else if (fixedDp) {
        out << ".000";
    }
    
#if (__GNUC__ < 3)
    out << std::ends;
#endif

    std::string s = out.str();

    return s;
}

RealTime
RealTime::operator/(int d) const
{
    int secdiv = sec / d;
    int secrem = sec % d;

    double nsecdiv = (double(nsec) + ONE_BILLION * double(secrem)) / d;
    
    return RealTime(secdiv, int(nsecdiv + 0.5));
}

double 
RealTime::operator/(const RealTime &r) const
{
    double lTotal = double(sec) * ONE_BILLION + double(nsec);
    double rTotal = double(r.sec) * ONE_BILLION + double(r.nsec);
    
    if (rTotal == 0) return 0.0;
    else return lTotal/rTotal;
}

long
RealTime::realTime2Frame(const RealTime &time, unsigned int sampleRate)
{
    if (time < zeroTime) return -realTime2Frame(-time, sampleRate);
    double s = time.sec + double(time.nsec) / ONE_BILLION;
    return long(s * sampleRate + 0.5);
}

RealTime
RealTime::frame2RealTime(long frame, unsigned int sampleRate)
{
    if (frame < 0) return -frame2RealTime(-frame, sampleRate);

    int sec = int(frame / long(sampleRate));
    frame -= sec * long(sampleRate);
    int nsec = (int)((double(frame) / double(sampleRate)) * ONE_BILLION + 0.5);
    return RealTime(sec, nsec);
}

const RealTime RealTime::zeroTime(0,0);

}

_VAMP_SDK_PLUGSPACE_END(RealTime.cpp)
