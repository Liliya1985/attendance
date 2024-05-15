#include <iostream>
#include <string>
#include <ctime>
#include <set>

using namespace std;

// Line indexes for MS Teams *.csv reports
const int LINE_MEETING_START = 3;
const int LINE_MEETING_END = 4;
const int LINE_MEETING_ATTENDANTS = 7;

// Column indexes for MS Teams *.csv reports
const int COL_NAME = 0;
const int COL_TOTAL_TIME = 3;

// Date format for MS Teams *.csv reports
const string DATE_FORMAT_FULL = "%d.%m.%Y, %H:%M:%S";
const string DATE_FORMAT_DATE = "%Y-%m-%d";

// Abbreviations used in MS Teams *.csv reports for duration of attendance (currently in Polish only)
const set<string> ABBR_HOUR = {"godz."};
const set<string> ABBR_MIN = {"min"};
const set<string> ABBR_SEC = {"sek."};

// Column names for the output csv (currently in Polish only)
const string LOC_NAME = "Osoba";
const string LOC_PRESENT = "PRAWDA";
const string LOC_NOTPRESENT = "FAŁSZ";

// Minimum threshold for counting the meeting as attended (e.g. 0.5 means that the person must be present for at least 50% of the duration of the meeting)
const double THRESHOLD = 0.5;

const string ERROR_CANNOT_PARSE_FILE = "Could not parse file {}. This meeting will not be included in the statistics.";
const string ERROR_INVALID_ARGUMENT = "Invalid argument. Perhaps you used a forbidden character in the name of the output file?";
const string ERROR_NO_DEFAULT_FOLDER = "The default folder with CSV reports does not exist.";
const string ERROR_NO_PATH_TO_FOLDER = "No path to folder with CSV reports. Attempting default path (.{})...";
const string ERROR_NO_FILENAME_IN_ARGUMENTS = "No filename specified. Saving with default naming convention...";

const string FILENAME = "attendance";
time_t now = time(0);
tm *ltm = localtime(&now);
const string DEFAULT_OUTPUT_FILENAME = to_string(1900 + ltm->tm_year) + "-" + to_string(1 + ltm->tm_mon) + "-" + to_string(ltm->tm_mday) + "-" + FILENAME + ".csv";

const string CSV_DIALECT = "excel-tab";
const string ENCODING = "utf16";

const string DEFAULT_ATTENDANCE_LIST_PATH = "\\lists";

const string MANUAL = R"(
Usage:
    attendance <path_to_folder> <output_file_name>

<path_to_folder> - include full or relative path; currently tested in Windows 10 only
<output_file_name> - will automatically append *.csv extension

Examples:
    attendance C:\Users\username\Documents\attendance_lists attendance_summary
    attendance attendance_lists attendance_summary
)";


