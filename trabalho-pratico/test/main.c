#include "../include/catalog.h"
#include "../include/file_util.h"

#include <glib.h>
#include <stdio.h>


void assert_invalid_csv_loads_nothing(char *dataset_folder_path) {
    FILE *users_file = open_file_folder(dataset_folder_path, "users.csv");
    FILE *drivers_file = open_file_folder(dataset_folder_path, "drivers.csv");
    FILE *rides_file = open_file_folder(dataset_folder_path, "rides.csv");

    char *line = malloc(sizeof(char) * 1000);

    gboolean failed = FALSE;

    fgets(line, 1000, users_file); // skip first line
    while (fgets(line, 1000, users_file) != NULL) {
        format_fgets_input_line(line);
        char *line_copy = g_strdup(line);
        User *user = parse_line_user(line_copy, ';');
        if (user != NULL) {
            printf("User should've been NULL for line '%s'\n", line);
            failed = TRUE;
            free_user(user);
        }
        free(line_copy);
    }

    fgets(line, 1000, drivers_file); // skip first line
    while (fgets(line, 1000, drivers_file) != NULL) {
        format_fgets_input_line(line);
        char *line_copy = g_strdup(line);
        Driver *driver = parse_line_driver(line_copy, ';');
        if (driver != NULL) {
            printf("Driver should've been NULL for line '%s'\n", line);
            failed = TRUE;
            free_driver(driver);
        }
        free(line_copy);
    }

    fgets(line, 1000, rides_file); // skip first line
    while (fgets(line, 1000, rides_file) != NULL) {
        format_fgets_input_line(line);
        char *line_copy = g_strdup(line);
        Ride *ride = parse_line_ride(line_copy, ';');
        if (ride != NULL) {
            printf("Ride should've been NULL for line '%s'\n", line);
            failed = TRUE;
            free_ride(ride);
        }
        free(line_copy);
    }

    if (failed) {
        g_test_fail();
    }
}

void assert_invalid_csv_loads_nothing_regular(void) {
    assert_invalid_csv_loads_nothing("datasets/data-regular-errors-invalid");
}

void assert_invalid_csv_loads_nothing_large(void) {
    assert_invalid_csv_loads_nothing("datasets/data-large-errors-invalid");
}

void assert_valid_csv_loads_everything_regular(void) {
    char *dataset_folder_path = "datasets/data-regular-errors-valid";

    FILE *users_file = open_file_folder(dataset_folder_path, "users.csv");
    FILE *drivers_file = open_file_folder(dataset_folder_path, "drivers.csv");
    FILE *rides_file = open_file_folder(dataset_folder_path, "rides.csv");

    char *line = malloc(sizeof(char) * 1000);

    gboolean failed = FALSE;

    fgets(line, 1000, users_file); // skip first line
    while (fgets(line, 1000, users_file) != NULL) {
        format_fgets_input_line(line);
        char *line_copy = g_strdup(line);
        User *user = parse_line_user(line_copy, ';');
        if (user == NULL) {
            g_test_message("User should've been non-NULL for line '%s'\n", line);
            failed = TRUE;
        }
        free_user(user);
        free(line_copy);
    }

    fgets(line, 1000, drivers_file); // skip first line
    while (fgets(line, 1000, drivers_file) != NULL) {
        format_fgets_input_line(line);
        char *line_copy = g_strdup(line);
        Driver *driver = parse_line_driver(line_copy, ';');
        if (driver == NULL) {
            g_test_message("Driver should've been non-NULL for line '%s'\n", line);
            failed = TRUE;
        }
        free_driver(driver);
        free(line_copy);
    }

    fgets(line, 1000, rides_file); // skip first line
    while (fgets(line, 1000, rides_file) != NULL) {
        format_fgets_input_line(line);
        char *line_copy = g_strdup(line);
        Ride *ride = parse_line_ride(line_copy, ';');
        if (ride == NULL) {
            g_test_message("Ride should've been non-NULL for line '%s'\n", line);
            failed = TRUE;
        }
        free_ride(ride);
        free(line_copy);
    }

    if (failed) {
        g_test_fail();
    }
}

int main(int argc, char *argv[]) {
    g_test_init(&argc, &argv, NULL);

    g_test_set_nonfatal_assertions();

    g_test_add_func("/assert_invalid_csv_loads_nothing_large", assert_invalid_csv_loads_nothing_large);
    g_test_add_func("/assert_invalid_csv_loads_nothing_regular", assert_invalid_csv_loads_nothing_regular);
    g_test_add_func("/assert_valid_csv_loads_everything", assert_valid_csv_loads_everything_regular);

    return g_test_run();
}
