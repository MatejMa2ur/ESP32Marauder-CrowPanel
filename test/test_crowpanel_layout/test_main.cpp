#include <unity.h>

#include "CrowPanelLayout.h"

using namespace crowpanel;

void test_header_hits() {
  TEST_ASSERT_EQUAL(HeaderHit::Back, headerHit(560, 20, true));
  TEST_ASSERT_EQUAL(HeaderHit::None, headerHit(560, 20, false));
  TEST_ASSERT_EQUAL(HeaderHit::Stop, headerHit(700, 20, true));
  TEST_ASSERT_EQUAL(HeaderHit::Stop, headerHit(700, 20, false));
  TEST_ASSERT_EQUAL(HeaderHit::None, headerHit(100, 20, true));
  TEST_ASSERT_EQUAL(HeaderHit::None, headerHit(560, 52, true));
}

void test_footer_hits() {
  TEST_ASSERT_EQUAL(FooterHit::Prev, footerHit(20, 410, 3));
  TEST_ASSERT_EQUAL(FooterHit::Next, footerHit(700, 410, 3));
  TEST_ASSERT_EQUAL(FooterHit::None, footerHit(400, 410, 3));
  TEST_ASSERT_EQUAL(FooterHit::None, footerHit(20, 410, 1));
  TEST_ASSERT_EQUAL(FooterHit::None, footerHit(700, 410, 0));
}

void test_row_at() {
  for (uint8_t i = 0; i < kRowsPerPage; i++) {
    const Rect r = rowRect(i);
    TEST_ASSERT_EQUAL_INT8(i, rowAt(r.x + r.w / 2, r.y + r.h / 2));
    TEST_ASSERT_EQUAL_INT8(i, rowAt(r.x, r.y));
    TEST_ASSERT_EQUAL_INT8(i, rowAt(r.x + r.w - 1, r.y + r.h - 1));
    TEST_ASSERT_EQUAL_INT8(-1, rowAt(r.x + 10, r.y + r.h));
    TEST_ASSERT_EQUAL_INT8(-1, rowAt(r.x - 1, r.y + 10));
    TEST_ASSERT_EQUAL_INT8(-1, rowAt(r.x + r.w, r.y + 10));
  }
  TEST_ASSERT_EQUAL_INT8(-1, rowAt(100, 71));
  TEST_ASSERT_EQUAL_INT8(-1, rowAt(100, 400));
}

void test_toolbar_hits() {
  TEST_ASSERT_EQUAL(ToolbarHit::ChDown, toolbarHit(30, 80));
  TEST_ASSERT_EQUAL(ToolbarHit::None, toolbarHit(150, 80));
  TEST_ASSERT_EQUAL(ToolbarHit::ChUp, toolbarHit(240, 80));
  TEST_ASSERT_EQUAL(ToolbarHit::Hop, toolbarHit(350, 80));
  TEST_ASSERT_EQUAL(ToolbarHit::None, toolbarHit(350, 120));
}

void test_page_count() {
  TEST_ASSERT_EQUAL_UINT16(1, pageCount(0));
  TEST_ASSERT_EQUAL_UINT16(1, pageCount(1));
  TEST_ASSERT_EQUAL_UINT16(1, pageCount(6));
  TEST_ASSERT_EQUAL_UINT16(2, pageCount(7));
  TEST_ASSERT_EQUAL_UINT16(3, pageCount(13));
  TEST_ASSERT_EQUAL_UINT16(12, pageStart(2));
  TEST_ASSERT_EQUAL_UINT16(2, pageOf(13));
  TEST_ASSERT_EQUAL_UINT16(0, pageOf(5));
}

void test_clamp_page() {
  TEST_ASSERT_EQUAL_UINT16(0, clampPage(-1, 3));
  TEST_ASSERT_EQUAL_UINT16(0, clampPage(-100000, 3));
  TEST_ASSERT_EQUAL_UINT16(1, clampPage(1, 3));
  TEST_ASSERT_EQUAL_UINT16(2, clampPage(3, 3));
  TEST_ASSERT_EQUAL_UINT16(2, clampPage(100000, 3));
  TEST_ASSERT_EQUAL_UINT16(0, clampPage(5, 0));
}

void test_layout_bounds() {
  for (uint8_t i = 0; i < kRowsPerPage; i++) {
    const Rect r = rowRect(i);
    TEST_ASSERT_TRUE(r.y >= kHeaderH);
    TEST_ASSERT_TRUE(r.y + r.h <= kPrevBtn.y);
  }
  TEST_ASSERT_TRUE(kBackBtn.y + kBackBtn.h <= kHeaderH);
  TEST_ASSERT_TRUE(kStopBtn.y + kStopBtn.h <= kHeaderH);
  TEST_ASSERT_TRUE(kBackBtn.x + kBackBtn.w <= kStopBtn.x);
  TEST_ASSERT_EQUAL_UINT8(25, scanTextLines(false));
  TEST_ASSERT_EQUAL_UINT8(22, scanTextLines(true));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_header_hits);
  RUN_TEST(test_footer_hits);
  RUN_TEST(test_row_at);
  RUN_TEST(test_toolbar_hits);
  RUN_TEST(test_page_count);
  RUN_TEST(test_clamp_page);
  RUN_TEST(test_layout_bounds);
  return UNITY_END();
}
