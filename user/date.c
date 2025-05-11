#include "kernel/types.h"
#include "user/user.h"

#define NS_PER_SEC 1000000000L
#define MS_PER_SEC 1000L
#define SEC_PER_DAY 86400
#define EPOCH_YEAR 1970

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millis;
} Date;

static const int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

static int is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static Date convert_time(uint64 ns) {
    Date date = {0};
    long seconds = ns / NS_PER_SEC;
    long nanos = ns % NS_PER_SEC;

    date.millis = nanos / (NS_PER_SEC / MS_PER_SEC);

    int rem = seconds % SEC_PER_DAY;
    date.hour = rem / 3600;
    date.minute = (rem % 3600) / 60;
    date.second = rem % 60;

    int days = seconds / SEC_PER_DAY;
    int year = EPOCH_YEAR;

    while (days >= (is_leap(year) ? 366 : 365)) {
        days -= is_leap(year) ? 366 : 365;
        year++;
    }
    date.year = year;

    int month = 0;
    int dims[12];
    memmove(dims, days_in_month, sizeof(dims));
    if (is_leap(year)) dims[1] = 29;

    while (month < 12 && days >= dims[month]) {
        days -= dims[month];
        month++;
    }
    date.month = month + 1;
    date.day = days + 1;

    return date;
}

static void fmt_str(char *buf, int value, int width) {
    for (int i = width-1; i >= 0; i--) {
        buf[i] = '0' + (value % 10);
        value /= 10;
    }
    buf[width] = 0;
}

int main() {
    uint64 ts = rtctime();
    Date dt = convert_time(ts);

    char y[5], m[3], d[3], h[3], min[3], sec[3], ms[4];

    fmt_str(y, dt.year, 4);
    fmt_str(m, dt.month, 2);
    fmt_str(d, dt.day, 2);
    fmt_str(h, dt.hour, 2);
    fmt_str(min, dt.minute, 2);
    fmt_str(sec, dt.second, 2);
    fmt_str(ms, dt.millis, 3);

    printf("%s-%s-%s %s:%s:%s.%s\n", y, m, d, h, min, sec, ms);
}