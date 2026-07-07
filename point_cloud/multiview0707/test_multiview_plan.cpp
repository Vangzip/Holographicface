#include "multiviewRenderPlan.h"
#include "memoryFrameSink.h"
#include "multiviewGraphicsConfig.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

static void test_target_parameters_produce_expected_counts() {
    MultiviewRenderPlan plan(90, 3, 150);

    assert(plan.samplesPerAxis() == 270);
    assert(plan.frameCount() == 72900);
    assert(plan.channels() == 3);
    assert(plan.frameBytes() == 67500);
    assert(plan.totalBytes() == 4920750000ULL);
}

static void test_frame_index_mapping_is_row_major() {
    MultiviewRenderPlan plan(90, 3, 150);

    MultiviewFrame first = plan.frameAt(0);
    assert(first.row == 0);
    assert(first.column == 0);
    assert(first.byteOffset == 0);

    MultiviewFrame second = plan.frameAt(1);
    assert(second.row == 0);
    assert(second.column == 1);
    assert(second.byteOffset == plan.frameBytes());

    MultiviewFrame row2first = plan.frameAt(270);
    assert(row2first.row == 1);
    assert(row2first.column == 0);
    assert(row2first.byteOffset == 270ULL * plan.frameBytes());

    MultiviewFrame last = plan.frameAt(72899);
    assert(last.row == 269);
    assert(last.column == 269);
    assert(last.byteOffset == (plan.frameCount() - 1) * plan.frameBytes());
}

static void test_invalid_parameters_are_rejected() {
    bool rejected = false;
    try {
        MultiviewRenderPlan plan(0, 3, 150);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        MultiviewRenderPlan plan(90, 0, 150);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        MultiviewRenderPlan plan(90, 3, 0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
}

static void test_overflow_parameters_are_rejected() {
    bool rejected = false;
    try {
        MultiviewRenderPlan plan(std::numeric_limits<int>::max(), 2, 1);
    } catch (const std::length_error&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        MultiviewRenderPlan plan(90000, 90000, 150);
    } catch (const std::length_error&) {
        rejected = true;
    }
    assert(rejected);
}

static void test_out_of_range_frame_is_rejected() {
    MultiviewRenderPlan plan(90, 3, 150);

    bool rejected = false;
    try {
        plan.frameAt(plan.frameCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    assert(rejected);
}

static void test_memory_sink_reports_layout_without_per_frame_objects() {
    MultiviewRenderPlan plan(2, 2, 3);
    MemoryFrameSink sink(plan, false);

    assert(sink.frameCount() == 16);
    assert(sink.frameBytes() == 27);
    assert(sink.totalBytes() == 432);
    assert(sink.data() == nullptr);
}

static void test_memory_sink_allocates_contiguous_slots() {
    MultiviewRenderPlan plan(2, 2, 3);
    MemoryFrameSink sink(plan, true);

    unsigned char* first = sink.frameData(0);
    unsigned char* second = sink.frameData(1);
    unsigned char* last = sink.frameData(15);

    assert(first != nullptr);
    assert(second == first + sink.frameBytes());
    assert(last == first + (sink.frameCount() - 1) * sink.frameBytes());

    first[0] = 7;
    last[sink.frameBytes() - 1] = 9;
    assert(sink.data()[0] == 7);
    assert(sink.data()[sink.totalBytes() - 1] == 9);
}

static void test_memory_sink_rejects_out_of_range_frame() {
    MultiviewRenderPlan plan(2, 2, 3);
    MemoryFrameSink sink(plan, true);

    bool rejected = false;
    try {
        sink.frameData(16);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    assert(rejected);
}

static void test_memory_graphics_config_avoids_window_swap() {
    MultiviewGraphicsConfig config = makeMultiviewGraphicsConfig(true);

    assert(config.windowDecoration == false);
    assert(config.doubleBuffer == false);
    assert(config.pbuffer == true);
    assert(config.vsync == false);
    assert(config.drawBuffer == MultiviewDrawBufferFront);
    assert(config.readBuffer == MultiviewDrawBufferFront);
}

static void test_legacy_graphics_config_keeps_visible_window() {
    MultiviewGraphicsConfig config = makeMultiviewGraphicsConfig(false);

    assert(config.windowDecoration == true);
    assert(config.doubleBuffer == true);
    assert(config.pbuffer == false);
    assert(config.vsync == false);
    assert(config.drawBuffer == MultiviewDrawBufferBack);
    assert(config.readBuffer == MultiviewDrawBufferBack);
}

int main() {
    test_target_parameters_produce_expected_counts();
    test_frame_index_mapping_is_row_major();
    test_invalid_parameters_are_rejected();
    test_overflow_parameters_are_rejected();
    test_out_of_range_frame_is_rejected();
    test_memory_sink_reports_layout_without_per_frame_objects();
    test_memory_sink_allocates_contiguous_slots();
    test_memory_sink_rejects_out_of_range_frame();
    test_memory_graphics_config_avoids_window_swap();
    test_legacy_graphics_config_keeps_visible_window();

    std::cout << "multiview plan tests passed\n";
    return 0;
}
