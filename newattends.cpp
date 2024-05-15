#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <ctime>
#include <iomanip>

using namespace std;
namespace fs = std::filesystem;

const string ENCODING = "utf-8";
const string DATE_FORMAT_FULL = "%Y-%m-%d %H:%M:%S";
const string DATE_FORMAT_DATE = "%Y-%m-%d";
const int LINE_MEETING_START = 1;
const int LINE_MEETING_END = 2;
const int LINE_MEETING_ATTENDANTS = 3;
const int COL_NAME = 0;
const int COL_TOTAL_TIME = 1;
const double THRESHOLD = 0.5;
const string ERROR_CANNOT_PARSE_FILE = "Cannot parse file: {}";
const string ERROR_NO_PATH_TO_FOLDER = "No path to folder provided: {}";
const string ERROR_NO_DEFAULT_FOLDER = "No default folder found.";
const string ERROR_NO_FILENAME_IN_ARGUMENTS = "No filename provided in arguments.";
const string ERROR_INVALID_ARGUMENT = "Invalid argument.";
const string DEFAULT_ATTENDANCE_LIST_PATH = "/default/path";
const string DEFAULT_OUTPUT_FILENAME = "output.csv";
const string LOC_NAME = "Name";
const string LOC_PRESENT = "Present";
const string LOC_NOTPRESENT = "Not Present";
const string MANUAL = "Manual";

vector<string> ABBR_HOUR = {"h", "hr", "hrs"};
vector<string> ABBR_MIN = {"m", "min", "mins"};
vector<string> ABBR_SEC = {"s", "sec", "secs"};

int get_duration_in_minutes(const string& meeting_duration) {
    vector<string> duration;
    stringstream ss(meeting_duration);
    string item;
    while (ss >> item) {
        duration.push_back(item);
    }

    int total_minutes = 0;
    for (size_t i = 0; i < duration.size(); ++i) {
        string value = duration[i];
        if (!all_of(value.begin(), value.end(), ::isdigit)) {
            continue;
        }

        if (find(ABBR_HOUR.begin(), ABBR_HOUR.end(), duration[i + 1]) != ABBR_HOUR.end()) {
            total_minutes += 60 * stoi(value);
        } else if (find(ABBR_MIN.begin(), ABBR_MIN.end(), duration[i + 1]) != ABBR_MIN.end()) {
            total_minutes += stoi(value);
        } else if (find(ABBR_SEC.begin(), ABBR_SEC.end(), duration[i + 1]) != ABBR_SEC.end()) {
            total_minutes += 1 / 60 * stoi(value);
        }
    }

    return total_minutes;
}

vector<string> get_attendance_filepaths(const string& folder_path) {
    vector<string> paths_to_files;
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            paths_to_files.push_back(entry.path().string());
        }
    }
    return paths_to_files;
}

tuple<unordered_map<string, int>, int, string> get_statistics_single_meeting(const string& csv_file) {
    ifstream csvfile(csv_file);
    if (!csvfile.is_open()) {
        throw runtime_error("Cannot open file: " + csv_file);
    }

    unordered_map<string, int> attendance_statistics;
    string line;
    int line_count = 0;
    tm meeting_start = {};
    tm meeting_end = {};

    while (getline(csvfile, line)) {
        stringstream ss(line);
        string token;
        vector<string> tokens;
        while (getline(ss, token, '\t')) {
            tokens.push_back(token);
        }

        if (line_count == LINE_MEETING_START) {
            istringstream ss(tokens[1]);
            ss >> get_time(&meeting_start, DATE_FORMAT_FULL.c_str());
        } else if (line_count == LINE_MEETING_END) {
            istringstream ss(tokens[1]);
            ss >> get_time(&meeting_end, DATE_FORMAT_FULL.c_str());
        } else if (line_count > LINE_MEETING_ATTENDANTS) {
            string person_name = tokens[COL_NAME];
            int time_attending = get_duration_in_minutes(tokens[COL_TOTAL_TIME]);
            attendance_statistics[person_name] += time_attending;
        }
        ++line_count;
    }

    time_t start_time = mktime(&meeting_start);
    time_t end_time = mktime(&meeting_end);
    int meeting_duration = difftime(end_time, start_time) / 60;

    char buffer[11];
    strftime(buffer, sizeof(buffer), DATE_FORMAT_DATE.c_str(), &meeting_start);
    string meeting_date(buffer);

    return make_tuple(attendance_statistics, meeting_duration, meeting_date);
}

tuple<unordered_map<string, vector<string>>, vector<string>> get_statistics_all_meetings(const vector<string>& paths_to_files) {
    unordered_map<string, vector<string>> attended_meetings;
    vector<string> meeting_dates;

    for (const auto& file : paths_to_files) {
        try {
            auto [attendance_statistics, meeting_duration, meeting_date] = get_statistics_single_meeting(file);
            meeting_dates.push_back(meeting_date);

            for (const auto& [person, time_attending] : attendance_statistics) {
                if (static_cast<double>(time_attending) / meeting_duration > THRESHOLD) {
                    attended_meetings[person].push_back(meeting_date);
                }
            }
        } catch (const exception& e) {
            cerr << ERROR_CANNOT_PARSE_FILE << file << endl;
        }
    }

    sort(meeting_dates.begin(), meeting_dates.end());
    return make_tuple(attended_meetings, meeting_dates);
}

vector<string> sort_attendants_by_last_name(const vector<string>& attendants) {
    vector<pair<string, string>> sorted_persons;

    for (const auto& person : attendants) {
        stringstream ss(person);
        string first_name, last_name;
        ss >> first_name >> last_name;
        sorted_persons.emplace_back(last_name, first_name);
    }

    sort(sorted_persons.begin(), sorted_persons.end(), [](const pair<string, string>& a, const pair<string, string>& b) {
        return a.first < b.first;
    });

    vector<string> sorted_attendants;
    for (const auto& [last_name, first_name] : sorted_persons) {
        sorted_attendants.push_back(first_name + " " + last_name);
    }

    return sorted_attendants;
}

void save_to_csv(const unordered_map<string, vector<string>>& attended_meetings, const vector<string>& meeting_dates, const string& filename) {
    ofstream csvfile(filename);
    if (!csvfile.is_open()) {
        throw runtime_error("Cannot open file: " + filename);
    }

    csvfile << LOC_NAME;
    for (const auto& date : meeting_dates) {
        csvfile << "," << date;
    }
    csvfile << endl;

    vector<string> persons;
    for (const auto& [person, _] : attended_meetings) {
        persons.push_back(person);
    }
    persons = sort_attendants_by_last_name(persons);

    for (const auto& person : persons) {
        csvfile << person;
        for (const auto& date : meeting_dates) {
            csvfile << "," << (find(attended_meetings.at(person).begin(), attended_meetings.at(person).end(), date) != attended_meetings.at(person).end() ? LOC_PRESENT : LOC_NOTPRESENT);
        }
        csvfile << endl;
    }
}

void show_manual_exit() {
    cout << MANUAL << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "help") {
        show_manual_exit();
    }

    vector<string> attendance_lists;
    try {
        attendance_lists = get_attendance_filepaths(argv[1]);
    } catch (const out_of_range&) {
        cerr << ERROR_NO_PATH_TO_FOLDER << DEFAULT_ATTENDANCE_LIST_PATH << endl;
        try {
            attendance_lists = get_attendance_filepaths(fs::current_path().string() + DEFAULT_ATTENDANCE_LIST_PATH);
        } catch (const fs::filesystem_error&) {
            cerr << ERROR_NO_DEFAULT_FOLDER << endl;
            show_manual_exit();
        }
    }

    string filename;
    try {
        filename = string(argv[2]) + ".csv";
    } catch (const out_of_range&) {
        cerr << ERROR_NO_FILENAME_IN_ARGUMENTS << endl;
        filename = DEFAULT_OUTPUT_FILENAME;
    }

    auto [attended_meetings, meeting_dates] = get_statistics_all_meetings(attendance_lists);

    try {
        save_to_csv(attended_meetings, meeting_dates, filename);
    } catch (const exception& e) {
        cerr << ERROR_INVALID_ARGUMENT << endl;
        show_manual_exit();
    }

    return 0;
}


