#include <unity.h>
#include "oui_db.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_apple_lookup() {
    uint8_t mac[] = {0x00, 0x17, 0xF2, 0x01, 0x02, 0x03};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Apple", v);
}

void test_espressif_lookup() {
    uint8_t mac[] = {0x18, 0xFE, 0x34, 0xAA, 0xBB, 0xCC};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Espressif", v);
}

void test_espressif_d4e9f4() {
    uint8_t mac[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x46, 0xB0};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Espressif", v);
}

void test_raspberry_pi() {
    uint8_t mac[] = {0xB8, 0x27, 0xEB, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Raspberry Pi", v);
}

void test_intel_lookup() {
    uint8_t mac[] = {0x00, 0x1B, 0x21, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Intel", v);
}

void test_samsung_lookup() {
    uint8_t mac[] = {0x00, 0x21, 0x19, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Samsung", v);
}

void test_unknown_oui() {
    uint8_t mac[] = {0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NULL(v);
}

void test_null_mac() {
    const char* v = ouiLookup(NULL);
    TEST_ASSERT_NULL(v);
}

void test_db_count() {
    TEST_ASSERT_GREATER_THAN(100, ouiCount());
}

void test_google_lookup() {
    uint8_t mac[] = {0x54, 0x60, 0x09, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Google", v);
}

void test_ubiquiti_lookup() {
    uint8_t mac[] = {0x00, 0x27, 0x22, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", v);
}

void test_realtek_lookup() {
    uint8_t mac[] = {0x00, 0xE0, 0x4C, 0x00, 0x00, 0x00};
    const char* v = ouiLookup(mac);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_STRING("Realtek", v);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_apple_lookup);
    RUN_TEST(test_espressif_lookup);
    RUN_TEST(test_espressif_d4e9f4);
    RUN_TEST(test_raspberry_pi);
    RUN_TEST(test_intel_lookup);
    RUN_TEST(test_samsung_lookup);
    RUN_TEST(test_unknown_oui);
    RUN_TEST(test_null_mac);
    RUN_TEST(test_db_count);
    RUN_TEST(test_google_lookup);
    RUN_TEST(test_ubiquiti_lookup);
    RUN_TEST(test_realtek_lookup);
    return UNITY_END();
}
