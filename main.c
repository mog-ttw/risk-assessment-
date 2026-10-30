    #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#define NAME_LENGTH 50
#define COURSE_LENGTH 40
#define STUDENT_ID_LENGTH 30
#define INITIAL_STUDENTS 10
#define PASS_MARK 50.0f

typedef struct {
    int id;
    char name[NAME_LENGTH];
    char course[COURSE_LENGTH];
    float grade;
} Student;

typedef struct {
    Student *students;
    size_t count;
    size_t capacity;
    char university_id[STUDENT_ID_LENGTH];
} App;

typedef void (*MenuFunction)(App *);
typedef int (*CompareFunction)(const void *, const void *);

static int read_line(const char *prompt, char *buffer, size_t size);
static int read_integer(const char *prompt, int *value);
static int read_grade(const char *prompt, float *value);
static int initialize_students(App *app);
static int resize_students(Student **students, size_t *capacity,
                           size_t required);
static int find_student_by_id(const App *app, int id);
static void print_header(const App *app);
static void print_student(const Student *student);
static const char *get_risk_level(float grade);

static void add_student(App *app);
static void display_students(App *app);
static void sort_students(App *app);
static void calculate_average(App *app);
static void find_top_student(App *app);
static void search_student(App *app);
static void remove_student(App *app);
static void custom_analysis(App *app);
static void generate_summary_report(App *app);
static void exit_program(App *app);

static int compare_by_grade(const void *a, const void *b);
static int compare_by_name(const void *a, const void *b);
static int compare_by_id(const void *a, const void *b);

int main(void)
{
    App app = {NULL, 0, 0, ""};
    int choice = 0;

    MenuFunction menu_functions[10] = {
        add_student,
        display_students,
        sort_students,
        calculate_average,
        find_top_student,
        search_student,
        remove_student,
        custom_analysis,
        generate_summary_report,
        exit_program
    };

    if (!read_line("Enter your university student ID: ",
                   app.university_id, sizeof(app.university_id)) ||
        app.university_id[0] == '\0') {
        fprintf(stderr, "A valid university student ID is required.\n");
        return EXIT_FAILURE;
    }

    print_header(&app);

    if (!initialize_students(&app)) {
        fprintf(stderr, "Unable to initialize the student database.\n");
        free(app.students);
        return EXIT_FAILURE;
    }

    do {
        printf("\n========== MAIN MENU ==========\n");
        printf("1. Add a new student\n");
        printf("2. Display all students\n");
        printf("3. Sort students\n");
        printf("4. Calculate average grade\n");
        printf("5. Find top-performing student\n");
        printf("6. Search for a student\n");
        printf("7. Remove a student\n");
        printf("8. Run custom academic analysis\n");
        printf("9. Generate summary report\n");
        printf("10. Exit\n");

        if (!read_integer("Enter your choice: ", &choice)) {
            printf("Invalid input. Enter a whole number.\n");
            continue;
        }

        if (choice < 1 || choice > 10) {
            printf("Invalid choice. Select a number from 1 to 10.\n");
            continue;
        }

        menu_functions[choice - 1](&app);

    } while (choice != 10);

    free(app.students);
    app.students = NULL;
    app.count = 0;
    app.capacity = 0;

    printf("\nAll allocated memory released.\n");
    printf("Program terminated successfully.\n");
    return EXIT_SUCCESS;
}

/* Read a complete line and reject input that does not fit the buffer. */
static int read_line(const char *prompt, char *buffer, size_t size)
{
    size_t length;
    int ch;

    if (size == 0) {
        return 0;
    }

    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        return 1;
    }

    /* If the line did not fit, discard the remainder and reject it. */
    ch = getchar();
    if (ch != '\n' && ch != EOF) {
        while (ch != '\n' && ch != EOF) {
            ch = getchar();
        }
        buffer[0] = '\0';
        return 0;
    }

    return 1;
}

static int read_integer(const char *prompt, int *value)
{
    char buffer[128];
    char *end;
    long result;

    if (!read_line(prompt, buffer, sizeof(buffer))) {
        return 0;
    }

    errno = 0;
    result = strtol(buffer, &end, 10);

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (buffer == end || *end != '\0' || errno == ERANGE ||
        result < -2147483647L - 1L || result > 2147483647L) {
        return 0;
    }

    *value = (int)result;
    return 1;
}

static int read_grade(const char *prompt, float *value)
{
    char buffer[128];
    char *end;
    float result;

    if (!read_line(prompt, buffer, sizeof(buffer))) {
        return 0;
    }

    errno = 0;
    result = strtof(buffer, &end);

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (buffer == end || *end != '\0' || errno == ERANGE ||
        !isfinite(result) || result < 0.0f || result > 100.0f) {
        return 0;
    }

    *value = result;
    return 1;
}

static int initialize_students(App *app)
{
    const Student initial_data[INITIAL_STUDENTS] = {
        {1001, "Daniel Aiyedogbon",  "Mathematics", 85.0f},
        {1002, "teller james",     "Programming", 48.0f},
        {1003, "shema Williams",  "Mathematics", 92.0f},
        {1004, "David nelson",     "Programming", 65.0f},
        {1005, "betty davinah",    "Database",    37.0f},
        {1006, "frenando Martinus",    "Database",    76.0f},
        {1007, "jason derula",    "Mathematics", 88.0f},
        {1008, "ishimwe tania",    "Programming", 55.0f},
        {1009, "Ikirezi deborah",     "Database",    95.0f},
        {1010, "Nshimiyimana bebe",    "Mathematics", 29.0f}
    };

    app->students = calloc(INITIAL_STUDENTS, sizeof(Student));
    if (app->students == NULL) {
        return 0;
    }

    memcpy(app->students, initial_data, sizeof(initial_data));
    app->count = INITIAL_STUDENTS;
    app->capacity = INITIAL_STUDENTS;
    return 1;
}

/* Double pointer lets this function update the caller's allocated array. */
static int resize_students(Student **students, size_t *capacity,
                           size_t required)
{
    Student *temporary;
    size_t new_capacity;

    if (required <= *capacity) {
        return 1;
    }

    new_capacity = (*capacity == 0) ? INITIAL_STUDENTS : *capacity;

    while (new_capacity < required) {
        if (new_capacity > ((size_t)-1) / 2) {
            return 0;
        }
        new_capacity *= 2;
    }

    if (new_capacity > ((size_t)-1) / sizeof(Student)) {
        return 0;
    }

    temporary = realloc(*students, new_capacity * sizeof(Student));
    if (temporary == NULL) {
        return 0;
    }

    *students = temporary;
    *capacity = new_capacity;
    return 1;
}

static int find_student_by_id(const App *app, int id)
{
    const Student *ptr;

    if (app->count == 0 || app->students == NULL) {
        return -1;
    }

    for (ptr = app->students; ptr < app->students + app->count; ptr++) {
        if (ptr->id == id) {
            return (int)(ptr - app->students);
        }
    }

    return -1;
}

static void add_student(App *app)
{
    Student new_student;

    printf("\n--- ADD NEW STUDENT ---\n");

    if (!read_integer("Enter student ID: ", &new_student.id) ||
        new_student.id <= 0) {
        printf("Invalid student ID. Use a positive whole number.\n");
        return;
    }

    if (find_student_by_id(app, new_student.id) != -1) {
        printf("Error: this student ID already exists.\n");
        return;
    }

    if (!read_line("Enter full name: ", new_student.name,
                   sizeof(new_student.name)) ||
        new_student.name[0] == '\0') {
        printf("Invalid or empty student name.\n");
        return;
    }

    if (!read_line("Enter course: ", new_student.course,
                   sizeof(new_student.course)) ||
        new_student.course[0] == '\0') {
        printf("Invalid or empty course name.\n");
        return;
    }

    if (!read_grade("Enter grade (0-100): ", &new_student.grade)) {
        printf("Invalid grade. Enter a number from 0 to 100.\n");
        return;
    }

    if (!resize_students(&app->students, &app->capacity, app->count + 1)) {
        printf("Memory allocation failed. Student was not added.\n");
        return;
    }

    app->students[app->count] = new_student;
    app->count++;
    printf("Student added successfully.\n");
}

static void print_student(const Student *student)
{
    printf("%-8d %-23s %-18s %7.2f\n",
           student->id, student->name, student->course, student->grade);
}

static void display_students(App *app)
{
    const Student *ptr;

    if (app->count == 0) {
        printf("No student records available.\n");
        return;
    }

    printf("\n================ STUDENT RECORDS ================\n");
    printf("%-8s %-23s %-18s %7s\n",
           "ID", "NAME", "COURSE", "GRADE");
    printf("---------------------------------------------------------------\n");

    for (ptr = app->students; ptr < app->students + app->count; ptr++) {
        print_student(ptr);
    }
}

static int compare_by_grade(const void *a, const void *b)
{
    const Student *s1 = (const Student *)a;
    const Student *s2 = (const Student *)b;

    if (s1->grade < s2->grade) return 1;
    if (s1->grade > s2->grade) return -1;
    return 0;
}

static int compare_by_name(const void *a, const void *b)
{
    const Student *s1 = (const Student *)a;
    const Student *s2 = (const Student *)b;
    return strcmp(s1->name, s2->name);
}

static int compare_by_id(const void *a, const void *b)
{
    const Student *s1 = (const Student *)a;
    const Student *s2 = (const Student *)b;

    if (s1->id < s2->id) return -1;
    if (s1->id > s2->id) return 1;
    return 0;
}

static void sort_students(App *app)
{
    int choice;
    CompareFunction comparators[3] = {
        compare_by_grade, compare_by_name, compare_by_id
    };

    if (app->count < 2) {
        printf("At least two students are needed for sorting.\n");
        return;
    }

    printf("\n--- SORT STUDENTS ---\n");
    printf("1. Grade (highest first)\n");
    printf("2. Name (A to Z)\n");
    printf("3. Student ID (ascending)\n");

    if (!read_integer("Choose sorting method: ", &choice) ||
        choice < 1 || choice > 3) {
        printf("Invalid sorting choice.\n");
        return;
    }

    qsort(app->students, app->count, sizeof(Student),
          comparators[choice - 1]);
    printf("Students sorted successfully.\n");
    display_students(app);
}

static void calculate_average(App *app)
{
    const Student *ptr;
    double total = 0.0;

    if (app->count == 0) {
        printf("No records available.\n");
        return;
    }

    for (ptr = app->students; ptr < app->students + app->count; ptr++) {
        total += ptr->grade;
    }

    printf("Class average: %.2f\n", total / (double)app->count);
}

static void find_top_student(App *app)
{
    const Student *ptr;
    const Student *top;

    if (app->count == 0) {
        printf("No student records available.\n");
        return;
    }

    top = app->students;

    for (ptr = app->students + 1; ptr < app->students + app->count; ptr++) {
        if (ptr->grade > top->grade) {
            top = ptr;
        }
    }

    printf("\n--- TOP-PERFORMING STUDENT ---\n");
    printf("%-8s %-23s %-18s %7s\n",
           "ID", "NAME", "COURSE", "GRADE");
    print_student(top);
}

static void search_student(App *app)
{
    int choice;
    int id;
    int index;
    int found = 0;
    char name[NAME_LENGTH];
    const Student *ptr;

    if (app->count == 0) {
        printf("No student records available.\n");
        return;
    }

    printf("\n--- SEARCH STUDENT ---\n");
    printf("1. Search by student ID\n");
    printf("2. Search by exact full name\n");

    if (!read_integer("Choose search method: ", &choice) ||
        (choice != 1 && choice != 2)) {
        printf("Invalid search option.\n");
        return;
    }

    if (choice == 1) {
        if (!read_integer("Enter student ID: ", &id)) {
            printf("Invalid student ID.\n");
            return;
        }

        index = find_student_by_id(app, id);
        if (index >= 0) {
            printf("%-8s %-23s %-18s %7s\n",
                   "ID", "NAME", "COURSE", "GRADE");
            print_student(&app->students[index]);
        } else {
            printf("Student not found.\n");
        }
    } else {
        if (!read_line("Enter full student name: ", name, sizeof(name))) {
            printf("Invalid name input.\n");
            return;
        }

        for (ptr = app->students; ptr < app->students + app->count; ptr++) {
            if (strcmp(ptr->name, name) == 0) {
                if (!found) {
                    printf("%-8s %-23s %-18s %7s\n",
                           "ID", "NAME", "COURSE", "GRADE");
                }
                print_student(ptr);
                found = 1;
            }
        }

        if (!found) {
            printf("Student not found.\n");
        }
    }
}

static void remove_student(App *app)
{
    int id;
    int index;
    size_t i;

    if (app->count == 0) {
        printf("No student records available.\n");
        return;
    }

    if (!read_integer("Enter student ID to remove: ", &id)) {
        printf("Invalid student ID.\n");
        return;
    }

    index = find_student_by_id(app, id);
    if (index < 0) {
        printf("Student not found.\n");
        return;
    }

    printf("Removing student: %s\n", app->students[index].name);

    for (i = (size_t)index; i + 1 < app->count; i++) {
        app->students[i] = app->students[i + 1];
    }

    app->count--;

    if (app->count == 0) {
        free(app->students);
        app->students = NULL;
        app->capacity = 0;
    } else if (app->count <= app->capacity / 2) {
        size_t new_capacity = app->capacity / 2;
        Student *temporary;

        if (new_capacity < INITIAL_STUDENTS) {
            new_capacity = INITIAL_STUDENTS;
        }

        if (new_capacity < app->capacity) {
            temporary = realloc(app->students,
                                new_capacity * sizeof(Student));
            if (temporary != NULL) {
                app->students = temporary;
                app->capacity = new_capacity;
            }
            /* If shrinking fails, the old allocation remains valid. */
        }
    }

    printf("Student removed successfully.\n");
}

static const char *get_risk_level(float grade)
{
    if (grade >= 70.0f) {
        return "LOW RISK";
    } else if (grade >= PASS_MARK) {
        return "MODERATE RISK";
    }
    return "HIGH RISK";
}

/*
 * Custom analysis:
 * Academic Risk Index = 100 - grade.
 * Grades 70-100: low risk; 50-69.99: moderate risk;
 * grades below 50: high risk.
 */
static void custom_analysis(App *app)
{
    const Student *ptr;
    size_t low = 0;
    size_t moderate = 0;
    size_t high = 0;

    if (app->count == 0) {
        printf("No records available for analysis.\n");
        return;
    }

    printf("\n======= ACADEMIC RISK ANALYSIS =======\n");
    printf("%-8s %-23s %8s %12s %-15s\n",
           "ID", "NAME", "GRADE", "RISK INDEX", "RISK LEVEL");

    for (ptr = app->students; ptr < app->students + app->count; ptr++) {
        float risk_index = 100.0f - ptr->grade;

        printf("%-8d %-23s %8.2f %12.2f %-15s\n",
               ptr->id, ptr->name, ptr->grade,
               risk_index, get_risk_level(ptr->grade));

        if (ptr->grade >= 70.0f) {
            low++;
        } else if (ptr->grade >= PASS_MARK) {
            moderate++;
        } else {
            high++;
        }
    }

    printf("\n--- RISK DISTRIBUTION ---\n");
    printf("Low-risk students: %zu (%.2f%%)\n",
           low, (double)low / (double)app->count * 100.0);
    printf("Moderate-risk students: %zu (%.2f%%)\n",
           moderate, (double)moderate / (double)app->count * 100.0);
    printf("High-risk students: %zu (%.2f%%)\n",
           high, (double)high / (double)app->count * 100.0);
}

static void generate_summary_report(App *app)
{
    const Student *ptr;
    double total = 0.0;
    float highest;
    float lowest;
    size_t pass = 0;
    size_t fail = 0;
    size_t high_risk = 0;

    if (app->count == 0) {
        printf("No records available for summary.\n");
        return;
    }

    highest = app->students[0].grade;
    lowest = app->students[0].grade;

    for (ptr = app->students; ptr < app->students + app->count; ptr++) {
        total += ptr->grade;

        if (ptr->grade > highest) highest = ptr->grade;
        if (ptr->grade < lowest) lowest = ptr->grade;

        if (ptr->grade >= PASS_MARK) {
            pass++;
        } else {
            fail++;
            high_risk++;
        }
    }

    printf("\n======= ACADEMIC SUMMARY REPORT =======\n");
    printf("Total student records: %zu\n", app->count);
    printf("Average grade: %.2f\n", total / (double)app->count);
    printf("Highest grade: %.2f\n", highest);
    printf("Lowest grade: %.2f\n", lowest);
    printf("Students who passed: %zu\n", pass);
    printf("Students below pass mark: %zu\n", fail);
    printf("High-risk students: %zu\n", high_risk);
    printf("Pass threshold: %.2f\n", PASS_MARK);
    printf("Pass rate: %.2f%%\n",
           (double)pass / (double)app->count * 100.0);
}

static void exit_program(App *app)
{
    (void)app;
    printf("Exiting Academic Performance Engine...\n");
}

static void print_header(const App *app)
{
    printf("\n============================================\n");
    printf("Student ID: %s\n", app->university_id);
    printf("Project: Dynamic Academic Performance and Risk Analysis Engine\n");
    printf("Custom Analysis: Academic Risk Index\n");
    printf("============================================\n");
}
