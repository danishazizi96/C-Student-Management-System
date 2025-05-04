// main.cpp
// Interactive Student Management System using OOP in C++
// Features are grouped in the UI as separate sections:
//   - Student Management
//   - Course Management
//   - Enrollment
//   - Reporting
//   - Data Export
// An option to populate dummy data is provided so you can quickly generate test entries.
// Data is saved in CSV files in separate folders ("Students", "Courses", "Reports").

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <cctype>     // for isspace, isdigit, tolower
#include <exception>  // for exception

namespace fs = std::filesystem;
using namespace std;

// ----------------------------
// Custom Exception for Operation Cancellation (UPDATED)
// ----------------------------
class OperationCancelledException : public exception {
public:
    const char* what() const noexcept override {
        return "Operation cancelled by user.";
    }
};

// ----------------------------
// Utility Functions for Input Handling
// ----------------------------

// Function to convert a string to lowercase (UPDATED)
string toLower(const string &s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Function to trim whitespace from the start and end of a string (UPDATED)
string trim(const string &s) {
    auto start = s.begin();
    while (start != s.end() && isspace(*start))
        start++;
    auto end = s.end();
    do {
        end--;
    } while (distance(start, end) > 0 && isspace(*end));
    return string(start, end + 1);
}

// Get a non-empty input string from the user (UPDATED)
// If the user enters "ESC" (case-insensitive), an OperationCancelledException is thrown.
string getNonEmptyInput(const string &prompt) {
    string input;
    do {
        cout << prompt;
        getline(cin, input);
        input = trim(input);
        if (toLower(input) == "esc") { // UPDATED: Check for cancellation
            throw OperationCancelledException();
        }
        if (input.empty()) {
            cout << "Input cannot be empty. Please try again.\n";
        }
    } while (input.empty());
    return input;
}

// Get a valid student type input ("Undergraduate" or "Postgraduate") (UPDATED)
string getValidStudentType() {
    string type;
    while (true) {
        type = getNonEmptyInput("Enter student type (Undergraduate/Postgraduate): ");
        if (type == "Undergraduate" || type == "Postgraduate")
            break;
        else
            cout << "Invalid type. Please enter either 'Undergraduate' or 'Postgraduate'.\n";
    }
    return type;
}

// Check if a student ID is valid (format Sxxx, e.g., S001) (UPDATED)
bool isValidStudentID(const string &studentID) {
    if (studentID.size() != 4)
        return false;
    if (studentID[0] != 'S')
        return false;
    for (size_t i = 1; i < studentID.size(); ++i) {
        if (!isdigit(studentID[i]))
            return false;
    }
    return true;
}

// Get a valid student ID (ensuring the format Sxxx) (UPDATED)
string getValidStudentID() {
    string id;
    while (true) {
        id = getNonEmptyInput("Enter student ID (format Sxxx, e.g., S001): ");
        if (isValidStudentID(id))
            break;
        else
            cout << "Invalid student ID format. Please follow the format Sxxx (e.g., S001).\n";
    }
    return id;
}

// Helper function to safely read an integer choice from user input. (UPDATED)
int getValidChoice() {
    int choice;
    while (true) {
        cin >> choice;
        if (cin.fail()) {
            cin.clear(); // clear error state
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard invalid input
            cout << "Invalid input. Please enter a valid number: ";
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear newline
            break;
        }
    }
    return choice;
}

// ----------------------------
// Abstract Base Class: Student
// ----------------------------
class Student {
protected:
    string name;
    string studentID;
    vector<string> enrolledCourses;  // stores course codes
public:
    Student(const string &name, const string &studentID)
        : name(name), studentID(studentID) {}
    virtual ~Student() {}

    string getName() const { return name; }
    string getID() const { return studentID; }
    const vector<string>& getCourses() const { return enrolledCourses; }

    // Enroll in a course
    void addCourse(const string &courseCode) {
        if (find(enrolledCourses.begin(), enrolledCourses.end(), courseCode) == enrolledCourses.end())
            enrolledCourses.push_back(courseCode);
    }

    // Remove a course enrollment
    void removeCourse(const string &courseCode) {
        enrolledCourses.erase(remove(enrolledCourses.begin(), enrolledCourses.end(), courseCode), enrolledCourses.end());
    }

    // Pure virtual function to get student type (e.g., Undergraduate, Postgraduate)
    virtual string getType() const = 0;
};

// ----------------------------
// Derived Class: Undergraduate
// ----------------------------
class Undergraduate : public Student {
public:
    Undergraduate(const string &name, const string &studentID)
        : Student(name, studentID) {}
    virtual string getType() const override {
        return "Undergraduate";
    }
};

// ----------------------------
// Derived Class: Postgraduate
// ----------------------------
class Postgraduate : public Student {
public:
    Postgraduate(const string &name, const string &studentID)
        : Student(name, studentID) {}
    virtual string getType() const override {
        return "Postgraduate";
    }
};

// ----------------------------
// Class: Course
// ----------------------------
class Course {
private:
    string courseName;
    string courseCode;
    vector<string> enrolledStudentIDs; // storing student IDs
public:
    Course(const string &courseName, const string &courseCode)
        : courseName(courseName), courseCode(courseCode) {}

    string getCourseName() const { return courseName; }
    string getCourseCode() const { return courseCode; }
    const vector<string>& getEnrolledStudents() const { return enrolledStudentIDs; }

    // Add a student to the course
    void addStudent(const string &studentID) {
        if (find(enrolledStudentIDs.begin(), enrolledStudentIDs.end(), studentID) == enrolledStudentIDs.end())
            enrolledStudentIDs.push_back(studentID);
    }

    // Remove a student from the course
    void removeStudent(const string &studentID) {
        enrolledStudentIDs.erase(remove(enrolledStudentIDs.begin(), enrolledStudentIDs.end(), studentID), enrolledStudentIDs.end());
    }
};

// ----------------------------
// Class: StudentManagement
// ----------------------------
class StudentManagement {
private:
    vector<unique_ptr<Student>> students;
    vector<Course> courses;

    // Utility: Find student index by studentID
    int findStudentIndex(const string &studentID) {
        for (size_t i = 0; i < students.size(); ++i) {
            if (students[i]->getID() == studentID)
                return static_cast<int>(i);
        }
        return -1;
    }

    // Utility: Find course index by courseCode
    int findCourseIndex(const string &courseCode) {
        for (size_t i = 0; i < courses.size(); ++i) {
            if (courses[i].getCourseCode() == courseCode)
                return static_cast<int>(i);
        }
        return -1;
    }

    // Ensure directory exists
    void ensureDirectory(const string &dirName) {
        if (!fs::exists(dirName))
            fs::create_directories(dirName);
    }

public:
    // ----------------------------
    // Student Management Functions
    // ----------------------------
    void addStudent(const string &name, const string &studentID, const string &type) {
        // Prevent duplicate student IDs.
        if (findStudentIndex(studentID) != -1) {
            cout << "Student with ID " << studentID << " already exists.\n";
            return;
        }
        if (type == "Undergraduate")
            students.push_back(make_unique<Undergraduate>(name, studentID));
        else if (type == "Postgraduate")
            students.push_back(make_unique<Postgraduate>(name, studentID));
        else {
            cout << "Unknown student type. Please use 'Undergraduate' or 'Postgraduate'.\n";
            return;
        }
        cout << "Student added: " << name << " (" << type << ")\n";
    }

    void removeStudent(const string &studentID) {
        int idx = findStudentIndex(studentID);
        if (idx == -1) {
            cout << "Student with ID " << studentID << " not found.\n";
            return;
        }
        // Remove student from any enrolled courses
        for (auto &course : courses)
            course.removeStudent(studentID);
        students.erase(students.begin() + idx);
        cout << "Student removed: " << studentID << "\n";
    }

    void listStudents() const {
        cout << "\n--- List of Students ---\n";
        for (const auto &student : students) {
            cout << "Name: " << student->getName()
                 << ", ID: " << student->getID()
                 << ", Type: " << student->getType() << "\n";
        }
    }

    void searchStudent(const string &keyword) const {
        cout << "\n--- Search Results for \"" << keyword << "\" ---\n";
        bool found = false;
        for (const auto &student : students) {
            if (student->getName().find(keyword) != string::npos ||
                student->getID().find(keyword) != string::npos) {
                cout << "Name: " << student->getName()
                     << ", ID: " << student->getID()
                     << ", Type: " << student->getType() << "\n";
                found = true;
            }
            else {
                // Also search in enrolled courses
                const auto &courseList = student->getCourses();
                for (const auto &course : courseList) {
                    if (course.find(keyword) != string::npos) {
                        cout << "Name: " << student->getName()
                             << ", ID: " << student->getID()
                             << ", Type: " << student->getType() << "\n";
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found)
            cout << "No matching student found.\n";
    }

    // ----------------------------
    // Course Management Functions
    // ----------------------------
    void addCourse(const string &courseName, const string &courseCode) {
        if (findCourseIndex(courseCode) != -1) {
            cout << "Course with code " << courseCode << " already exists.\n";
            return;
        }
        courses.emplace_back(courseName, courseCode);
        cout << "Course added: " << courseName << " (" << courseCode << ")\n";
    }

    void removeCourse(const string &courseCode) {
        int idx = findCourseIndex(courseCode);
        if (idx == -1) {
            cout << "Course with code " << courseCode << " not found.\n";
            return;
        }
        // Remove course from students' enrolled lists
        for (auto &student : students)
            student->removeCourse(courseCode);
        courses.erase(courses.begin() + idx);
        cout << "Course removed: " << courseCode << "\n";
    }

    void listCourses() const {
        cout << "\n--- List of Courses ---\n";
        for (const auto &course : courses) {
            cout << "Course Name: " << course.getCourseName()
                 << ", Course Code: " << course.getCourseCode() << "\n";
        }
    }

    // ----------------------------
    // Enrollment Functions
    // ----------------------------
    void enrollStudentInCourse(const string &studentID, const string &courseCode) {
        int sIdx = findStudentIndex(studentID);
        int cIdx = findCourseIndex(courseCode);
        if (sIdx == -1) {
            cout << "Student with ID " << studentID << " not found.\n";
            return;
        }
        if (cIdx == -1) {
            cout << "Course with code " << courseCode << " not found.\n";
            return;
        }
        // UPDATED: Check if the student is already enrolled in the course.
        const auto &enrolled = students[sIdx]->getCourses();
        if (find(enrolled.begin(), enrolled.end(), courseCode) != enrolled.end()) {
            cout << "Student " << studentID << " is already enrolled in course " << courseCode << ".\n";
            return;
        }
        students[sIdx]->addCourse(courseCode);
        courses[cIdx].addStudent(studentID);
        cout << "Enrolled student " << studentID << " in course " << courseCode << "\n";
    }

    void removeStudentFromCourse(const string &studentID, const string &courseCode) {
        int sIdx = findStudentIndex(studentID);
        int cIdx = findCourseIndex(courseCode);
        if (sIdx == -1 || cIdx == -1) {
            cout << "Either student or course not found.\n";
            return;
        }
        students[sIdx]->removeCourse(courseCode);
        courses[cIdx].removeStudent(studentID);
        cout << "Removed student " << studentID << " from course " << courseCode << "\n";
    }

    // ----------------------------
    // Reporting Functions
    // ----------------------------
    // Generate report for a given course: displays on terminal and saves as CSV
    void generateReportForCourse(const string &courseCode) {
        int cIdx = findCourseIndex(courseCode);
        if (cIdx == -1) {
            cout << "Course with code " << courseCode << " not found.\n";
            return;
        }
        cout << "\n--- Course Report for " << courseCode << " ---\n";
        cout << "StudentID,Name,Type\n";

        string dir = "Reports/CourseReports";
        ensureDirectory(dir);
        string filename = dir + "/" + courseCode + ".csv";
        ofstream file(filename);
        if (!file) {
            cout << "Error opening file for report.\n";
            return;
        }
        file << "StudentID,Name,Type\n";

        const auto &studentIDs = courses[cIdx].getEnrolledStudents();
        for (const auto &id : studentIDs) {
            int sIdx = findStudentIndex(id);
            if (sIdx != -1) {
                string line = students[sIdx]->getID() + "," + students[sIdx]->getName() + "," + students[sIdx]->getType();
                cout << line << "\n";
                file << line << "\n";
            }
        }
        file.close();
        cout << "Course report saved to: " << filename << "\n";
    }

    // Generate report for a specific student: displays on terminal and saves as CSV
    // UPDATED: Includes student’s own information at the top of the CSV.
    void generateReportForStudent(const string &studentID) {
        int sIdx = findStudentIndex(studentID);
        if (sIdx == -1) {
            cout << "Student with ID " << studentID << " not found.\n";
            return;
        }
        // Display student information first
        cout << "\n--- Student Report for " << studentID << " ---\n";
        cout << "StudentID,Name,Type\n";
        cout << students[sIdx]->getID() << "," << students[sIdx]->getName() << "," << students[sIdx]->getType() << "\n\n";
        cout << "CourseCode,CourseName\n";

        string dir = "Reports/StudentReports";
        ensureDirectory(dir);
        string filename = dir + "/" + studentID + ".csv";
        ofstream file(filename);
        if (!file) {
            cout << "Error opening file for report.\n";
            return;
        }
        // Write student info at the top of the file
        file << "StudentID,Name,Type\n";
        file << students[sIdx]->getID() << "," << students[sIdx]->getName() << "," << students[sIdx]->getType() << "\n\n";
        file << "CourseCode,CourseName\n";

        const auto &courseCodes = students[sIdx]->getCourses();
        for (const auto &code : courseCodes) {
            int cIdx = findCourseIndex(code);
            if (cIdx != -1) {
                string line = courses[cIdx].getCourseCode() + "," + courses[cIdx].getCourseName();
                cout << line << "\n";
                file << line << "\n";
            }
        }
        file.close();
        cout << "Student report saved to: " << filename << "\n";
    }

    // ----------------------------
    // Data Export Functions
    // ----------------------------
    void exportStudentsToCSV() {
        string dir = "Students";
        ensureDirectory(dir);
        string filename = dir + "/students.csv";
        ofstream file(filename);
        if (!file) {
            cout << "Error opening file for exporting students.\n";
            return;
        }
        file << "StudentID,Name,Type,EnrolledCourses\n";
        for (const auto &student : students) {
            file << student->getID() << ","
                 << student->getName() << ","
                 << student->getType() << ",";
            // Concatenate enrolled courses (separated by ;)
            const auto &coursesList = student->getCourses();
            for (size_t i = 0; i < coursesList.size(); ++i) {
                file << coursesList[i];
                if (i != coursesList.size() - 1)
                    file << ";";
            }
            file << "\n";
        }
        file.close();
        cout << "Students exported to " << filename << "\n";
    }

    void exportCoursesToCSV() {
        string dir = "Courses";
        ensureDirectory(dir);
        string filename = dir + "/courses.csv";
        ofstream file(filename);
        if (!file) {
            cout << "Error opening file for exporting courses.\n";
            return;
        }
        file << "CourseCode,CourseName,EnrolledStudents\n";
        for (const auto &course : courses) {
            file << course.getCourseCode() << ","
                 << course.getCourseName() << ",";
            const auto &studentsList = course.getEnrolledStudents();
            for (size_t i = 0; i < studentsList.size(); ++i) {
                file << studentsList[i];
                if (i != studentsList.size() - 1)
                    file << ";";
            }
            file << "\n";
        }
        file.close();
        cout << "Courses exported to " << filename << "\n";
    }
    
    // ----------------------------
    // Data Loading Functions (Persistence)
    // ----------------------------
    // Load students from CSV file
    void loadStudentsFromCSV() {
        string filename = "Students/students.csv";
        ifstream file(filename);
        if (!file) {
            // File may not exist on first run
            return;
        }
        string line;
        getline(file, line); // Skip header
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string studentID, name, type, coursesStr;
            getline(ss, studentID, ',');
            getline(ss, name, ',');
            getline(ss, type, ',');
            getline(ss, coursesStr);
            
            addStudent(name, studentID, type); // addStudent() already checks for duplicates

            // Split coursesStr by ';' and add them
            if (!coursesStr.empty()) {
                stringstream courseStream(coursesStr);
                string courseCode;
                while (getline(courseStream, courseCode, ';')) {
                    int sIdx = findStudentIndex(studentID);
                    if (sIdx != -1)
                        students[sIdx]->addCourse(courseCode);
                }
            }
        }
        file.close();
    }

    // Load courses from CSV file
    void loadCoursesFromCSV() {
        string filename = "Courses/courses.csv";
        ifstream file(filename);
        if (!file) {
            // File may not exist on first run
            return;
        }
        string line;
        getline(file, line); // Skip header
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string courseCode, courseName, studentsStr;
            getline(ss, courseCode, ',');
            getline(ss, courseName, ',');
            getline(ss, studentsStr);

            addCourse(courseName, courseCode);

            // Split studentsStr by ';' and add them
            if (!studentsStr.empty()) {
                stringstream stuStream(studentsStr);
                string stuID;
                while (getline(stuStream, stuID, ';')) {
                    int cIdx = findCourseIndex(courseCode);
                    if (cIdx != -1)
                        courses[cIdx].addStudent(stuID);
                }
            }
        }
        file.close();
    }

    // Wrapper function to load both students and courses.
    void loadData() {
        loadStudentsFromCSV();
        loadCoursesFromCSV();
    }

    // ----------------------------
    // Dummy Data Population
    // ----------------------------
    void populateDummyData() {
        // Add dummy students
        addStudent("Alice Johnson", "S001", "Undergraduate");
        addStudent("Bob Smith", "S002", "Postgraduate");
        addStudent("Charlie Brown", "S003", "Undergraduate");
        addStudent("David Williams", "S004", "Undergraduate");
        addStudent("Eve Davis", "S005", "Postgraduate");
        
        // Add dummy courses
        addCourse("Introduction to Programming", "CSE101");
        addCourse("Data Structures", "CSE102");
        addCourse("Algorithms", "CSE103");
        addCourse("Operating Systems", "CSE104");
        
        // Create enrollments
        enrollStudentInCourse("S001", "CSE101");
        enrollStudentInCourse("S001", "CSE102");
        enrollStudentInCourse("S002", "CSE101");
        enrollStudentInCourse("S003", "CSE103");
        enrollStudentInCourse("S004", "CSE104");
        enrollStudentInCourse("S005", "CSE101");
        enrollStudentInCourse("S005", "CSE102");
        enrollStudentInCourse("S005", "CSE104");
        
        cout << "\nDummy data populated successfully.\n";
    }
};

// ----------------------------
// Main: Interactive Menu
// ----------------------------
int main() {
    StudentManagement sms;
    // Load previously saved data (if any) to ensure persistence
    sms.loadData();
    
    int choice;
    
    do {
        cout << "\n==============================\n";
        cout << "     Student Management\n";
        cout << "==============================\n";
        cout << "1. Add Student\n";
        cout << "2. Remove Student\n";
        cout << "3. List Students\n";
        cout << "4. Search Student\n\n";
        
        cout << "==============================\n";
        cout << "      Course Management\n";
        cout << "==============================\n";
        cout << "5. Add Course\n";
        cout << "6. Remove Course\n";
        cout << "7. List Courses\n\n";
        
        cout << "==============================\n";
        cout << "         Enrollment\n";
        cout << "==============================\n";
        cout << "8. Enroll Student in Course\n";
        cout << "9. Remove Student from Course\n\n";
        
        cout << "==============================\n";
        cout << "         Reporting\n";
        cout << "==============================\n";
        cout << "10. Generate Course Report\n";
        cout << "11. Generate Student Report\n\n";
        
        cout << "==============================\n";
        cout << "         Data Export\n";
        cout << "==============================\n";
        cout << "12. Export Data to CSV (Students & Courses)\n\n";
        
        cout << "==============================\n";
        cout << "       Populate Dummy Data\n";
        cout << "==============================\n";
        cout << "13. Populate Dummy Data\n\n";
        
        cout << "==============================\n";
        cout << "              Exit\n";
        cout << "==============================\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
        choice = getValidChoice();
        
        try {
            switch(choice) {
                case 1: {
                    // Get validated input for student addition.
                    // For student ID, we enforce the Sxxx format.
                    string name = getNonEmptyInput("Enter student name: ");
                    string studentID = getValidStudentID(); // UPDATED: Ensure proper format.
                    string type = getValidStudentType();
                    sms.addStudent(name, studentID, type);
                    break;
                }
                case 2: {
                    string studentID = getNonEmptyInput("Enter student ID to remove: ");
                    sms.removeStudent(studentID);
                    break;
                }
                case 3: {
                    sms.listStudents();
                    break;
                }
                case 4: {
                    string keyword = getNonEmptyInput("Enter keyword to search (name, ID, or course code): ");
                    sms.searchStudent(keyword);
                    break;
                }
                case 5: {
                    string courseName = getNonEmptyInput("Enter course name: ");
                    string courseCode = getNonEmptyInput("Enter course code: ");
                    sms.addCourse(courseName, courseCode);
                    break;
                }
                case 6: {
                    string courseCode = getNonEmptyInput("Enter course code to remove: ");
                    sms.removeCourse(courseCode);
                    break;
                }
                case 7: {
                    sms.listCourses();
                    break;
                }
                case 8: {
                    string studentID = getNonEmptyInput("Enter student ID to enroll: ");
                    string courseCode = getNonEmptyInput("Enter course code to enroll in: ");
                    sms.enrollStudentInCourse(studentID, courseCode);
                    break;
                }
                case 9: {
                    string studentID = getNonEmptyInput("Enter student ID to remove from course: ");
                    string courseCode = getNonEmptyInput("Enter course code: ");
                    sms.removeStudentFromCourse(studentID, courseCode);
                    break;
                }
                case 10: {
                    string courseCode = getNonEmptyInput("Enter course code for report: ");
                    sms.generateReportForCourse(courseCode);
                    break;
                }
                case 11: {
                    string studentID = getNonEmptyInput("Enter student ID for report: ");
                    sms.generateReportForStudent(studentID);
                    break;
                }
                case 12: {
                    sms.exportStudentsToCSV();
                    sms.exportCoursesToCSV();
                    break;
                }
                case 13: {
                    sms.populateDummyData();
                    break;
                }
                case 0: {
                    cout << "Exiting the system. Goodbye!\n";
                    // Export data on exit to preserve changes
                    sms.exportStudentsToCSV();
                    sms.exportCoursesToCSV();
                    break;
                }
                default: {
                    cout << "Invalid choice. Please try again.\n";
                    break;
                }
            }
        }
        catch (const OperationCancelledException &e) {
            cout << e.what() << "\n"; // Inform the user and return to main menu.
        }
        
    } while (choice != 0);

    return 0;
}
