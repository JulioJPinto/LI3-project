#include <printf.h>
#include "catalog.h"
#include "price_util.h"

#include "catalog_sort.h"

struct DriverbyCity {
    int id;
    char *name;
    int accumulated_score;
    int amount_rides;
};

DriverbyCity *create_driver_by_city(int id, char *name) {
    DriverbyCity *driver_by_city = malloc(sizeof(DriverbyCity));
    driver_by_city->id = id;
    driver_by_city->name = g_strdup(name);

    driver_by_city->accumulated_score = 0;
    driver_by_city->amount_rides = 0;
    return driver_by_city;
}

int driver_by_city_get_id(DriverbyCity *driver) {
    return driver->id;
}

char *driver_by_city_get_name(DriverbyCity *driver) {
    return driver->name;
}

void driver_by_city_increment_number_of_rides(DriverbyCity *driver) {
    driver->amount_rides++;
}

void driver_by_city_add_score(DriverbyCity *driver, int score) {
    driver->accumulated_score += score;
}

double driver_by_city_get_average_score(DriverbyCity *driver) {
    return (double) driver->accumulated_score / (double) driver->amount_rides;
}

//typedef enum {ARRAY, HASH} GHashArray_Type;

typedef union {
    GPtrArray *drivers_in_city_array;
    GHashTable *drivers_in_city_hash;
} GHashArray;

/**
 * Struct that represents a catalog.
 */
struct Catalog {
    GPtrArray *users_array;
    GPtrArray *drivers_array;
    GPtrArray *rides_array;

    GHashTable *user_from_username_hashtable; // key: username (char*), value: User*
    GHashTable *driver_from_id_hashtable;     // key: driver id (int), value: Driver*

    GHashTable *rides_in_city_hashtable; // key: city (char*), value: GPtrArray of rides
    GHashTable *drivers_in_city;         // key: city (char*), value: GPtrArray DriverbyCity

    //GhashTable <City, Union<GHashTable<id, Struct(Driver, Score)>>, Array...>
};

/**
 * Function that wraps free user to be used in GLib g_ptr_array free func.
 */
void glib_wrapper_free_user(gpointer user) {
    free_user(user);
}

/**
 * Function that wraps free driver to be used in GLib g_ptr_array free func.
 */
void glib_wrapper_free_driver(gpointer driver) {
    free_driver(driver);
}

/**
 * Function that wraps free ride to be used in GLib g_ptr_array free func.
 */
void glib_wrapper_free_ride(gpointer ride) {
    free_ride(ride);
}

/**
 * Function that wraps free array to be used in GLib g_hash_table free func.
 */
void glib_wrapper_ptr_array_free_segment(gpointer array) {
    g_ptr_array_free(array, TRUE);
}

Catalog *create_catalog(void) {
    Catalog *catalog = malloc(sizeof(struct Catalog));

    catalog->users_array = g_ptr_array_new_full(100000, glib_wrapper_free_user);
    catalog->drivers_array = g_ptr_array_new_full(10000, glib_wrapper_free_driver);
    catalog->rides_array = g_ptr_array_new_full(1000000, glib_wrapper_free_ride);

    catalog->user_from_username_hashtable = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    catalog->driver_from_id_hashtable = g_hash_table_new_full(g_int_hash, g_int_equal, free, NULL);

    catalog->rides_in_city_hashtable = g_hash_table_new_full(g_str_hash, g_str_equal, free, glib_wrapper_ptr_array_free_segment);
    catalog->drivers_in_city = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);

    return catalog;
}

void free_catalog(Catalog *catalog) {
    g_ptr_array_free(catalog->users_array, TRUE);
    g_ptr_array_free(catalog->drivers_array, TRUE);
    g_ptr_array_free(catalog->rides_array, TRUE);

    g_hash_table_destroy(catalog->user_from_username_hashtable);
    g_hash_table_destroy(catalog->driver_from_id_hashtable);
    g_hash_table_destroy(catalog->rides_in_city_hashtable);
    g_hash_table_destroy(catalog->drivers_in_city);

    free(catalog);
}

void register_user(Catalog *catalog, User *user) {
    g_ptr_array_add(catalog->users_array, user);

    char *key = user_get_username(user);
    // No need to free the key, it's freed by the hashtable when the user is removed

    g_hash_table_insert(catalog->user_from_username_hashtable, key, user);
}

void register_driver(Catalog *catalog, Driver *driver) {
    g_ptr_array_add(catalog->drivers_array, driver);

    int *key = malloc(sizeof(int));
    *key = driver_get_id(driver);
    // No need to free the key, it will be freed when the Driver is freed

    g_hash_table_insert(catalog->driver_from_id_hashtable, key, driver);
}

/**
 * Internal function of register_ride that indexes the city of the ride.
 */
static void catalog_ride_index_city(Catalog *catalog, Ride *ride) {
    char *city = ride_get_city(ride); // Only needs to be freed if the city is already in the hashtable

    GPtrArray *rides_in_city;
    if (g_hash_table_contains(catalog->rides_in_city_hashtable, city)) {
        rides_in_city = g_hash_table_lookup(catalog->rides_in_city_hashtable, city);
        free(city);
    } else {
        rides_in_city = g_ptr_array_new();
        g_hash_table_insert(catalog->rides_in_city_hashtable, city, rides_in_city);
    }

    g_ptr_array_add(rides_in_city, ride);
}

void insert_new_driver_city(int driver_id, GHashTable *driver_by_city, Catalog *catalog) {
    Driver *driver = catalog_get_driver(catalog, driver_id);
    DriverbyCity *new_driver = create_driver_by_city(driver_id, driver_get_name(driver));
    int *key = malloc(sizeof(int));
    *key = driver_id;
    g_hash_table_insert(driver_by_city, key, new_driver);
}

void add_score_to_driver_city(int score, int driver_id, GHashTable *driver_by_city, Catalog *catalog) {
    DriverbyCity *driver = g_hash_table_lookup(driver_by_city, &driver_id);
    if (driver == NULL) { 

        insert_new_driver_city(driver_id, driver_by_city, catalog);
        driver = g_hash_table_lookup(driver_by_city, &driver_id);
    }
    driver_by_city_add_score(driver, score);
    driver_by_city_increment_number_of_rides(driver);
}


void catalog_add_driver_by_city(Catalog *catalog, Ride *ride, char *city) {
    GHashTable *driver_in_city_hash = g_hash_table_lookup(catalog->drivers_in_city, city);
    int driver_id = ride_get_driver_id(ride);

    if (driver_in_city_hash == NULL) {
        driver_in_city_hash = g_hash_table_new_full(g_int_hash, g_int_equal, free, NULL);
        g_hash_table_insert(catalog->drivers_in_city, city, driver_in_city_hash);
    } 
    

    add_score_to_driver_city(ride_get_score_driver(ride), driver_id, driver_in_city_hash, catalog);
}

void register_ride(Catalog *catalog, Ride *ride) {
    g_ptr_array_add(catalog->rides_array, ride);
    char *city = ride_get_city(ride);

    Driver *driver = catalog_get_driver(catalog, ride_get_driver_id(ride));
    double price = compute_price(ride_get_distance(ride), driver_get_car_class(driver));
    ride_set_price(ride, price);

    double total_price = ride_get_tip(ride) + price;

    driver_increment_number_of_rides(driver);
    driver_add_score(driver, ride_get_score_driver(ride));
    driver_add_earned(driver, total_price);
    driver_register_ride_date(driver, ride_get_date(ride));

    char *username = ride_get_user_username(ride);

    User *user = catalog_get_user(catalog, username);
    user_increment_number_of_rides(user);
    user_add_score(user, ride_get_score_user(ride));
    user_add_spent(user, total_price);
    user_add_total_distance(user, ride_get_distance(ride));
    user_register_ride_date(user, ride_get_date(ride));

    free(username);

    catalog_ride_index_city(catalog, ride);
    catalog_add_driver_by_city(catalog, ride, city);
}

User *catalog_get_user(Catalog *catalog, char *username) {
    return g_hash_table_lookup(catalog->user_from_username_hashtable, username);
}

Driver *catalog_get_driver(Catalog *catalog, int id) {
    return g_hash_table_lookup(catalog->driver_from_id_hashtable, &id);
}

int catalog_get_top_drivers_with_best_score(Catalog *catalog, int n, GPtrArray *result) {
    // TODO: todo in catalog_get_top_users_with_longest_total_distance
    int length = MIN(n, (int) catalog->drivers_array->len);

    for (int i = 0; i < length; i++) {
        g_ptr_array_add(result, g_ptr_array_index(catalog->drivers_array, i));
    }

    return length;
}

int catalog_get_top_users_with_longest_total_distance(Catalog *catalog, int n, GPtrArray *result) {
    // TODO: This can be improved, no need to copy the array, just create a new one pointing to the users_array with the length of n
    int length = MIN(n, (int) catalog->users_array->len);

    for (int i = 0; i < n; i++) {
        g_ptr_array_add(result, g_ptr_array_index(catalog->users_array, i));
    }

    return length;
}

double catalog_get_average_price_in_city(Catalog *p_catalog, char *city) {
    GPtrArray *rides = g_hash_table_lookup(p_catalog->rides_in_city_hashtable, city);
    if (rides == NULL) return 0;

    double total_price = 0;

    // Performance impact of this loop is negligible, even with ~200000 rides per city.
    // We also assume that the user won't ask this query twice for the same city, so no need to cache.

    for (guint i = 0; i < rides->len; i++) {
        Ride *ride = g_ptr_array_index(rides, i);
        total_price += ride_get_price(ride);
    }

    return total_price / rides->len;
}

/**
 * Returns the index of the lowest ride whose date is greater than the given date.
 */
guint ride_array_find_date_lower_bound(GPtrArray *array, Date date) {
    guint mid;

    guint low = 0;
    guint high = array->len;

    while (low < high) {
        mid = low + (high - low) / 2;

        if (date_compare(date, ride_get_date(g_ptr_array_index(array, mid))) <= 0) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    // TODO: maybe stop binary search when finds a date that is equal to
    //  the given date and then do a linear search backwards to find the lower bound

    return low;
}

/*
 * This can also be done with a hashtable that keeps the accumulated price for each date,
 * do two binary searches for the lower and upper bounds and return the difference.
 * (maybe not worth it)
 */
double catalog_get_average_price_in_date_range(Catalog *catalog, Date start_date, Date end_date) {
    GPtrArray *rides = catalog->rides_array;

    long current_value_index = ride_array_find_date_lower_bound(rides, start_date);

    double total_price = 0;
    int rides_count = 0;

    while (current_value_index < rides->len) {
        Ride *current_ride = g_ptr_array_index(rides, current_value_index);
        Date current_ride_date = ride_get_date(current_ride);

        if (date_compare(current_ride_date, end_date) > 0)
            break;

        total_price += ride_get_price(current_ride);
        rides_count++;

        current_value_index++;
    }

    // divide by zero check
    return rides_count != 0 ? total_price / rides_count : 0;
}

double catalog_get_average_distance_in_city_by_date(Catalog *catalog, Date start_date, Date end_date, char *city) {
    GPtrArray *rides_in_city = g_hash_table_lookup(catalog->rides_in_city_hashtable, city);
    if (rides_in_city == NULL) return 0;

    double total_distance = 0;
    int ride_count = 0;

    long current_value_index = ride_array_find_date_lower_bound(rides_in_city, start_date);

    while (current_value_index < rides_in_city->len) {
        Ride *current_ride = g_ptr_array_index(rides_in_city, current_value_index);
        Date current_ride_date = ride_get_date(current_ride);

        if (date_compare(current_ride_date, end_date) > 0)
            break;

        total_distance += ride_get_distance(current_ride);
        ride_count++;

        current_value_index++;
    }

    // divide by zero check
    return ride_count != 0 ? total_distance / ride_count : 0;
}

void catalog_insert_passengers_that_gave_tip_in_date_range(Catalog *catalog, GPtrArray *result, Date start_date, Date end_date) {
    GPtrArray *rides = catalog->rides_array;

    long current_value_index = ride_array_find_date_lower_bound(rides, start_date);

    Ride *current_ride;
    Date current_ride_date;

    while (current_value_index < rides->len) {
        current_ride = g_ptr_array_index(rides, current_value_index);
        current_ride_date = ride_get_date(current_ride);

        if (date_compare(current_ride_date, end_date) > 0)
            break;

        if (ride_get_tip(current_ride) > 0) {
            g_ptr_array_add(result, current_ride);
        }

        current_value_index++;
    }

    g_ptr_array_sort(result, glib_wrapper_compare_rides_by_distance);
}

/**
 * Sorts the value by date.
 * This is used to sort a hash table with value: array of rides.
 * @param key unused
 * @param value the array to be sorted
 * @param user_data unused
 */
void hash_table_sort_array_values_by_date(gpointer key, gpointer value, gpointer user_data) {
    (void) key;
    (void) user_data;

    g_ptr_array_sort(value, glib_wrapper_compare_rides_by_date);
}

int compare_driver_in_city_by_score(DriverbyCity *a, DriverbyCity *b) {
    double average_score_a = driver_by_city_get_average_score(a);
    double average_score_b = driver_by_city_get_average_score(b);

    return (average_score_a > average_score_b) - (average_score_a < average_score_b);
}

int compare_driver_in_city_by_id(DriverbyCity *a, DriverbyCity *b) {
    int a_driver_id = driver_by_city_get_id(a);
    int b_driver_id = driver_by_city_get_id(b);

    return a_driver_id - b_driver_id;
}

int glib_wrapper_compare_drivers_in_city_by_score(gconstpointer a, gconstpointer b) {
    DriverbyCity *a_driver = *(DriverbyCity **) a;
    DriverbyCity *b_driver = *(DriverbyCity **) b;

    int by_score = compare_driver_in_city_by_score(b_driver, a_driver);
    if (by_score != 0) {
        return by_score;
    }

    return compare_driver_in_city_by_id(b_driver, a_driver);
}

void hash_table_sort_array_values_by_score(gpointer key, gpointer value, gpointer user_data) {
    (void) key;
    (void) user_data;

    g_ptr_array_sort(value, glib_wrapper_compare_drivers_in_city_by_score);
}

void free_driver_by_city(DriverbyCity *driver) {
    free(driver->name);
    free(driver);
}

void glib_wrapper_free_driver_by_city(gpointer driver_by_city) {
    free_driver_by_city(*(DriverbyCity **) (driver_by_city));
}

GPtrArray *alter_hash_to_gptr_array(GHashTable *hastable) {
    GPtrArray *gptr_array_hash_table_keys = *(GPtrArray **) g_hash_table_get_keys_as_array(hastable, NULL);

    GPtrArray *gptr_array_hash_table_values = g_ptr_array_new_full(10000, glib_wrapper_free_driver_by_city);
    for (guint i = 0; gptr_array_hash_table_keys->len; i++) {
        g_ptr_array_add(gptr_array_hash_table_values, g_hash_table_lookup(hastable, gptr_array_hash_table_keys));
    }

    g_hash_table_destroy(hastable);
    g_ptr_array_free(gptr_array_hash_table_keys, TRUE);

    return gptr_array_hash_table_values;
}

void glib_wrapper_alter_hash_to_gptr_array(gpointer key, gpointer hash_array, gpointer user_data) {
    (void) key;
    (void) user_data;

    GHashArray *value = *(GHashArray **) hash_array;
    if (value == NULL) {
        return;
    }

    if (value->drivers_in_city_hash) {
        GPtrArray *gptr_array_values = alter_hash_to_gptr_array(value->drivers_in_city_hash);
        value->drivers_in_city_array = gptr_array_values;
    }
}

void notify_stop_registering(Catalog *catalog) {
    g_hash_table_foreach(catalog->drivers_in_city, glib_wrapper_alter_hash_to_gptr_array, NULL);

    g_ptr_array_sort(catalog->drivers_array, glib_wrapper_compare_drivers_by_score);
    g_ptr_array_sort(catalog->users_array, glib_wrapper_compare_users_by_total_distance);
    // Sort rides by date for faster query 5 that requires lookup in a date range
    g_ptr_array_sort(catalog->rides_array, glib_wrapper_compare_rides_by_date);

    // Sort each rides array in the rides_in_city_hashtable by date for faster query 6 that requires date range
    // TODO: Maybe make so the sort for each city is only done when a query for that city is called
    g_hash_table_foreach(catalog->rides_in_city_hashtable, hash_table_sort_array_values_by_date, NULL);
    g_hash_table_foreach(catalog->drivers_in_city, hash_table_sort_array_values_by_score, NULL);
}

void catalog_get_top_n_drivers_in_city(Catalog *catalog, int n, char *city, GPtrArray *result) {
    GPtrArray *top_drivers_in_city = g_hash_table_lookup(catalog->rides_in_city_hashtable, city); //Aqui
    if (top_drivers_in_city == NULL) return;

    for (guint i = 0; i < n; i++) {
        DriverbyCity *driver = g_ptr_array_index(top_drivers_in_city, i);
        g_ptr_array_add(result, driver);
    }
}
