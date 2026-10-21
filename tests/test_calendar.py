#!/usr/bin/env python3
"""Compare the firmware C implementation with Python's independent Gregorian/ISO calendar."""
import ctypes as C, datetime as dt, pathlib, subprocess, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
BUILD=ROOT/'build'; BUILD.mkdir(exist_ok=True)
subprocess.run(['gcc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','GUI/calendar.c','-o',str(BUILD/'calendar.so')],cwd=ROOT,check=True)
class Date(C.Structure):
    _fields_=[('year',C.c_uint16)]+[(f,C.c_uint8) for f in ('month','day','hour','minute','second')]
lib=C.CDLL(str(BUILD/'calendar.so'))
lib.cal_from_timestamp.argtypes=[C.c_uint32];lib.cal_from_timestamp.restype=Date
lib.cal_to_timestamp.argtypes=[Date];lib.cal_to_timestamp.restype=C.c_uint32
lib.cal_iso_week.argtypes=[Date,C.POINTER(C.c_uint16)];lib.cal_iso_week.restype=C.c_uint8
lib.cal_weekday.argtypes=[C.c_uint16,C.c_uint8,C.c_uint8]
lib.cal_month_days.argtypes=[C.c_uint16,C.c_uint8]
lib.cal_day_kind.argtypes=[C.c_uint16,C.c_uint8,C.c_uint8]
lib.cal_transfers_known.argtypes=[C.c_uint16];lib.cal_transfers_known.restype=C.c_bool
lib.cal_next_holiday.argtypes=[Date,C.POINTER(Date)];lib.cal_next_holiday.restype=C.c_char_p
lib.cal_day_of_year.argtypes=[Date];lib.cal_day_of_year.restype=C.c_uint16
lib.cal_days_left_in_year.argtypes=[Date];lib.cal_days_left_in_year.restype=C.c_uint16
class CalendarTests(unittest.TestCase):
    def test_every_day_1970_2105(self):
        epoch=dt.date(1970,1,1);day=epoch;count=0
        while day<dt.date(2106,1,1):
            t=(day-epoch).days*86400+86399
            got=lib.cal_from_timestamp(t)
            self.assertEqual((got.year,got.month,got.day,got.hour,got.minute,got.second),(day.year,day.month,day.day,23,59,59))
            self.assertEqual(lib.cal_to_timestamp(got),t)
            self.assertEqual(lib.cal_day_of_year(got),day.timetuple().tm_yday)
            self.assertEqual(lib.cal_days_left_in_year(got),(dt.date(day.year,12,31)-day).days)
            following=lib.cal_from_timestamp(t+1); tomorrow=day+dt.timedelta(days=1)
            self.assertEqual((following.year,following.month,following.day,following.hour,following.minute,following.second),(tomorrow.year,tomorrow.month,tomorrow.day,0,0,0))
            self.assertEqual(lib.cal_weekday(day.year,day.month,day.day),day.weekday())
            wy=C.c_uint16();week=lib.cal_iso_week(got,C.byref(wy))
            self.assertEqual((wy.value,week),day.isocalendar()[:2])
            day+=dt.timedelta(days=1);count+=1
        print(f'Validated {count} civil dates, midnight boundaries and ISO weeks')
    def test_leap(self):
        for y,days in [(2000,29),(2024,29),(2025,28),(2028,29),(2100,28)]:
            self.assertEqual(lib.cal_month_days(y,2),days)
    def test_russia_2026(self):
        for m,d in [(1,9),(3,9),(5,11),(12,31)]: self.assertEqual(lib.cal_day_kind(2026,m,d),3)
        for m,d in [(1,1),(1,7),(2,23),(3,8),(5,1),(5,9),(6,12),(11,4)]: self.assertEqual(lib.cal_day_kind(2026,m,d),2)
        count=0;date=dt.date(2026,1,1)
        while date.year==2026:
            count+=lib.cal_day_kind(date.year,date.month,date.day)==0;date+=dt.timedelta(days=1)
        self.assertEqual(count,247)
    def test_transferred_workdays(self):
        self.assertEqual(lib.cal_day_kind(2025,11,1),0)
        self.assertEqual(lib.cal_day_kind(2025,11,3),3)
        self.assertEqual(lib.cal_day_kind(2027,2,20),0)
        self.assertEqual(lib.cal_day_kind(2027,2,22),3)
        for year,expected in [(2025,247),(2027,247)]:
            date=dt.date(year,1,1);work=0
            while date.year==year:
                work+=lib.cal_day_kind(year,date.month,date.day)==0;date+=dt.timedelta(days=1)
            self.assertEqual(work,expected)
    def test_upcoming_holiday_dates(self):
        for current,expected in [((2026,10,21),(2026,11,4)),((2026,12,31),(2027,1,1)),((2027,1,2),(2027,1,7)),((2027,1,8),(2027,2,23))]:
            following=Date()
            self.assertTrue(lib.cal_next_holiday(Date(*current,0,0,0),C.byref(following)))
            self.assertEqual((following.year,following.month,following.day),expected)
    def test_unknown_year(self):
        self.assertFalse(lib.cal_transfers_known(2028))
        self.assertEqual(lib.cal_day_kind(2028,1,10),0)
        self.assertEqual(lib.cal_day_kind(2028,1,1),2)
if __name__=='__main__': unittest.main()
