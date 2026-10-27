#pragma once

#include <lvgl.h>
#include <cstdio>
#include "gravy_vegetable_assets.h"
#include "gravy_wifi.h"

// UI-only GravyMaker touchscreen for CrowPanel DIS08070H (800x480), LVGL 9.
// Spice readings below are mock values until the real ESP32/sensor integration.
static bool liveDispenseActive = false;
static bool liveDispenseFailed = false;
namespace gravyui {

static int spiceLevelsUI[12] = {
    -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1
};

static uint32_t BG       = 0xF4F6F8;
static uint32_t CARD     = 0xFFFFFF;
static uint32_t INK      = 0x17212B;
static uint32_t MUTED    = 0x687582;
static uint32_t ORANGE   = 0xE97735;
static uint32_t GREEN    = 0x26845B;
static uint32_t RED      = 0xC94C4C;
static uint32_t BORDER   = 0xDFE5E9;
static uint32_t PALE     = 0xFFF0E5;
static uint32_t PALEG    = 0xE5F4EC;
static uint32_t PALEY    = 0xFFF3D5;
static bool darkTheme = false;
static uint32_t BTN_NEUTRAL = 0xE7EBEF;
static uint32_t BTN_TEXT = 0x17212B;
static uint8_t currentPage = 0; // 0 home, 1 ingredients, 2 servings, 3 success, 4 spice check, 5 dispensing
static lv_timer_t *dispenseTimer = nullptr;
static lv_timer_t *spiceTimer = nullptr;
static int dispenseStage = 0;
static int dispenseStageCount = 0;
static bool dispenseFinished = false;
static bool dispenseOrderOnion[4] = {false, false, false, false};
static bool dispenseOrderTomato[4] = {false, false, false, false};
static bool dispenseOrderIsOnion[4] = {false, false, false, false};
static uint8_t dispenseOrderCount = 0;

static void apply_theme() {
    if (darkTheme) {
        BG = 0x111820; CARD = 0x202B36; INK = 0xF3F6F8; MUTED = 0xB1BEC9;
        ORANGE = 0xFF995C; GREEN = 0x55C995; RED = 0xFF7373; BORDER = 0x354351;
        PALE = 0x493326; PALEG = 0x203D32; PALEY = 0x453A20; BTN_NEUTRAL = 0x354351; BTN_TEXT = 0xF3F6F8;
    } else {
        BG = 0xF4F6F8; CARD = 0xFFFFFF; INK = 0x17212B; MUTED = 0x687582;
        ORANGE = 0xE97735; GREEN = 0x26845B; RED = 0xC94C4C; BORDER = 0xDFE5E9;
        PALE = 0xFFF0E5; PALEG = 0xE5F4EC; PALEY = 0xFFF3D5; BTN_NEUTRAL = 0xE7EBEF; BTN_TEXT = 0x17212B;
    }
}

static bool onion = false;
static bool tomato = false;
static uint8_t servings = 1;
static lv_obj_t *root = nullptr;
static lv_obj_t *onionCard = nullptr;
static lv_obj_t *tomatoCard = nullptr;
static lv_obj_t *oneCard = nullptr;
static lv_obj_t *twoCard = nullptr;
static lv_obj_t *spicePulse = nullptr;

// Mock spice status: 0=insufficient, 1=enough for 2 servings, 2=full.
// Replace these with sensor-derived statuses during later integration.
static uint8_t spiceStatus[12] = {2,1,0,2,1,2,0,1,2,1,0,2};

static lv_obj_t *label(lv_obj_t *parent, const char *txt, uint32_t color,
                       const lv_font_t *font = &lv_font_montserrat_14) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(l, font, 0);
    return l;
}

static lv_obj_t *button(lv_obj_t *parent, const char *txt, int w, int h,
                        uint32_t bg, uint32_t fg = CARD) {
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_radius(b, 15, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_t *l = label(b, txt, fg);
    lv_obj_center(l);
    return b;
}

static lv_obj_t *panel(lv_obj_t *parent, int w, int h) {
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_size(p, w, h);
    lv_obj_set_style_bg_color(p, lv_color_hex(CARD), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(p, 22, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(BORDER), 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_shadow_width(p, 12, 0);
    lv_obj_set_style_shadow_opa(p, LV_OPA_10, 0);
    lv_obj_set_style_pad_all(p, 18, 0);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    return p;
}

static void clear_screen() {
    if (dispenseTimer) { lv_timer_delete(dispenseTimer); dispenseTimer = nullptr; }
    if (spiceTimer) { lv_timer_delete(spiceTimer); spiceTimer = nullptr; }
    if (root) lv_obj_del(root);
    root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, 800, 480);
    lv_obj_set_style_bg_color(root, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
}

static void toggle_theme(lv_event_t *);

static void header(const char *section, const char *subtitle) {
    lv_obj_t *brand = label(root, "GRAVYMAKER", INK, &lv_font_montserrat_40);
    lv_obj_align(brand, LV_ALIGN_TOP_LEFT, 30, 18);
    lv_obj_t *tag = label(root, section, ORANGE);
    lv_obj_align(tag, LV_ALIGN_TOP_RIGHT, -155, 29);
    lv_obj_t *themeBtn = button(root, darkTheme ? "LIGHT MODE" : "DARK MODE", 112, 36,
                                BTN_NEUTRAL, BTN_TEXT);
    lv_obj_align(themeBtn, LV_ALIGN_TOP_RIGHT, -24, 18);
    lv_obj_add_event_cb(themeBtn, toggle_theme, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sub = label(root, subtitle, MUTED);
    lv_obj_align(sub, LV_ALIGN_TOP_LEFT, 34, 72);
}

static void fade_in() {
    // Visible but brief slide-and-fade transition between screens.
    lv_obj_set_style_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_translate_y(root, 14, 0);
    lv_anim_t fade;
    lv_anim_init(&fade);
    lv_anim_set_var(&fade, root);
    lv_anim_set_values(&fade, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&fade, 320);
    lv_anim_set_exec_cb(&fade, [](void *obj, int32_t v) {
        lv_obj_set_style_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(v), 0);
    });
    lv_anim_start(&fade);

    lv_anim_t slide;
    lv_anim_init(&slide);
    lv_anim_set_var(&slide, root);
    lv_anim_set_values(&slide, 14, 0);
    lv_anim_set_duration(&slide, 320);
    lv_anim_set_exec_cb(&slide, [](void *obj, int32_t v) {
        lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(obj), v, 0);
    });
    lv_anim_start(&slide);
}

static void show_home();
static void show_modules();
static void show_servings();
static void show_complete();
static void show_spices();
static void show_spice_loading();
static void finish_spice_loading(lv_timer_t *timer);
static void show_dispense_stage();
static void advance_dispense_stage(lv_timer_t *timer);
static void start_dispensing();

static void redraw_current_page() {
    switch (currentPage) {
        case 1: show_modules(); break;
        case 2: show_servings(); break;
        case 3: show_complete(); break;
        case 4: show_spices(); break;
        case 5: //Do no start old timer based simulation
                break;
        case 6: show_spice_loading(); break;
        default: show_home(); break;
    }
}
static void toggle_theme(lv_event_t *) {
    darkTheme = !darkTheme;
    apply_theme();
    redraw_current_page();
}
static void back_to_home(lv_event_t *) { show_home(); }
static void open_dispense(lv_event_t *) { onion = false; tomato = false; show_modules(); }
static void open_spices(lv_event_t *) {
    if (dispenseRequestActive()) {
        Serial.println("Another request is active.");
        return;
    }

    // Clear previous results before starting a fresh scan.
    for (int i = 0; i < 12; i++) {
        spiceLevelsUI[i] = -1;
    }

    if (beginSpiceCheckRequest()) {
        show_spice_loading();
    } else {
        currentPage = 6;
        clear_screen();
        header("SPICE CHECK ERROR", "Could not contact GravyMaker");

        lv_obj_t *msg = label(root, "CONNECTION FAILED", RED,
                              &lv_font_montserrat_40);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *back = button(root, "RETURN HOME", 220, 50, ORANGE);
        lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -25);
        lv_obj_add_event_cb(back, back_to_home, LV_EVENT_CLICKED, NULL);
    }
}
static void go_servings(lv_event_t *) {
    if (!onion && !tomato) return;
    show_servings();
}
static void choose_onion(lv_event_t *) { onion = !onion; show_modules(); }
static void choose_tomato(lv_event_t *) { tomato = !tomato; show_modules(); }
static void choose_one(lv_event_t *) { servings = 1; show_servings(); }
static void choose_two(lv_event_t *) { servings = 2; show_servings(); }
static void submit(lv_event_t *) { start_dispensing(); }

// Use the supplied real vegetable photos embedded in firmware.
static void vegetable_art(lv_obj_t *parent, bool isOnion) {
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, isOnion ? &gravy_onion_image : &gravy_tomato_image);
    lv_image_set_scale(img, 145); // about 57%, leaves room for the label
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 8);
}

static void show_home() {
    currentPage = 0;
    clear_screen(); header("HOME", "Choose what you would like to do");
    lv_obj_t *d = panel(root, 340, 275); lv_obj_align(d, LV_ALIGN_TOP_LEFT, 45, 130);
    lv_obj_t *s = panel(root, 340, 275); lv_obj_align(s, LV_ALIGN_TOP_RIGHT, -45, 130);
    // Animated mini dispenser illustration (no extra image/GIF downloads required).
    lv_obj_t *art = lv_obj_create(d); lv_obj_remove_style_all(art); lv_obj_set_size(art, 150, 100);
    lv_obj_set_style_bg_opa(art, LV_OPA_TRANSP, 0); lv_obj_align(art, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_t *hopper = lv_obj_create(art); lv_obj_remove_style_all(hopper); lv_obj_set_size(hopper, 58, 34);
    lv_obj_set_style_radius(hopper, 8, 0); lv_obj_set_style_bg_color(hopper, lv_color_hex(PALE), 0); lv_obj_set_style_bg_opa(hopper, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hopper, lv_color_hex(ORANGE), 0); lv_obj_set_style_border_width(hopper, 2, 0); lv_obj_align(hopper, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_t *body = lv_obj_create(art); lv_obj_remove_style_all(body); lv_obj_set_size(body, 82, 39);
    lv_obj_set_style_radius(body, 9, 0); lv_obj_set_style_bg_color(body, lv_color_hex(PALEG), 0); lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(body, lv_color_hex(GREEN), 0); lv_obj_set_style_border_width(body, 2, 0); lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, -7);
    lv_obj_t *window = lv_obj_create(body); lv_obj_remove_style_all(window); lv_obj_set_size(window, 44, 18); lv_obj_set_style_radius(window, 5, 0);
    lv_obj_set_style_bg_color(window, lv_color_hex(ORANGE), 0); lv_obj_set_style_bg_opa(window, LV_OPA_50, 0); lv_obj_align(window, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *drop = lv_obj_create(art); lv_obj_remove_style_all(drop); lv_obj_set_size(drop, 10, 10); lv_obj_set_style_radius(drop, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(drop, lv_color_hex(ORANGE), 0); lv_obj_set_style_bg_opa(drop, LV_OPA_COVER, 0); lv_obj_align(drop, LV_ALIGN_TOP_MID, 0, 42);
    lv_anim_t dropAnim; lv_anim_init(&dropAnim); lv_anim_set_var(&dropAnim, drop); lv_anim_set_values(&dropAnim, 0, 22); lv_anim_set_duration(&dropAnim, 700); lv_anim_set_playback_duration(&dropAnim, 400); lv_anim_set_repeat_count(&dropAnim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&dropAnim, [](void *v, int32_t n) { lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(v), n, 0); }); lv_anim_start(&dropAnim);
    lv_obj_t *dt = label(d, "DISPENSE", INK, &lv_font_montserrat_40); lv_obj_align(dt, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_t *dd = label(d, "Onion, tomato & servings", MUTED); lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 163);
    lv_obj_t *db = button(d, "OPEN DISPENSE  >", 245, 48, ORANGE); lv_obj_align(db, LV_ALIGN_BOTTOM_MID, 0, -4); lv_obj_add_event_cb(db, open_dispense, LV_EVENT_CLICKED, NULL);
    // Animated spice jar icon, with three floating spice dots.
    lv_obj_t *spiceIcon = lv_obj_create(s); lv_obj_remove_style_all(spiceIcon); lv_obj_set_size(spiceIcon, 130, 100);
    lv_obj_set_style_bg_opa(spiceIcon, LV_OPA_TRANSP, 0); lv_obj_align(spiceIcon, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_t *jar = lv_obj_create(spiceIcon); lv_obj_remove_style_all(jar); lv_obj_set_size(jar, 62, 67);
    lv_obj_set_style_radius(jar, 13, 0); lv_obj_set_style_bg_color(jar, lv_color_hex(PALEG), 0); lv_obj_set_style_bg_opa(jar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(jar, lv_color_hex(GREEN), 0); lv_obj_set_style_border_width(jar, 2, 0); lv_obj_align(jar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_t *lid = lv_obj_create(spiceIcon); lv_obj_remove_style_all(lid); lv_obj_set_size(lid, 38, 10);
    lv_obj_set_style_radius(lid, 4, 0); lv_obj_set_style_bg_color(lid, lv_color_hex(ORANGE), 0); lv_obj_set_style_bg_opa(lid, LV_OPA_COVER, 0); lv_obj_align_to(lid, jar, LV_ALIGN_OUT_TOP_MID, 0, 2);
    const uint32_t dotColors[3] = {0xE97735, 0xD99A18, 0x55C995};
    for (int i=0; i<3; ++i) { lv_obj_t *dot=lv_obj_create(spiceIcon); lv_obj_remove_style_all(dot); lv_obj_set_size(dot, 9, 9); lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0); lv_obj_set_style_bg_color(dot, lv_color_hex(dotColors[i]), 0); lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0); lv_obj_align(dot, LV_ALIGN_TOP_MID, (i-1)*24, 8); lv_anim_t bounce; lv_anim_init(&bounce); lv_anim_set_var(&bounce,dot); lv_anim_set_values(&bounce,0,-8-(i%2)*4); lv_anim_set_duration(&bounce,650+i*140); lv_anim_set_playback_duration(&bounce,650+i*140); lv_anim_set_repeat_count(&bounce,LV_ANIM_REPEAT_INFINITE); lv_anim_set_exec_cb(&bounce,[](void *v,int32_t n){lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(v),n,0);}); lv_anim_start(&bounce); }
    lv_obj_t *st = label(s, "CHECK SPICE", INK, &lv_font_montserrat_40); lv_obj_align(st, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_t *sd = label(s, "View all 12 compartments", MUTED); lv_obj_align(sd, LV_ALIGN_TOP_MID, 0, 163);
    lv_obj_t *sb = button(s, "VIEW SPICE LEVELS  >", 245, 48, GREEN); lv_obj_align(sb, LV_ALIGN_BOTTOM_MID, 0, -4); lv_obj_add_event_cb(sb, open_spices, LV_EVENT_CLICKED, NULL);
    fade_in();
}

static void show_modules() {
    currentPage = 1;
    clear_screen(); header("DISPENSE / 01", "Select one or both ingredients");
    lv_obj_t *back = button(root, "< BACK", 95, 38, BTN_NEUTRAL, BTN_TEXT); lv_obj_align(back, LV_ALIGN_TOP_LEFT, 30, 95); lv_obj_add_event_cb(back, back_to_home, LV_EVENT_CLICKED, NULL);
    onionCard = panel(root, 310, 265); lv_obj_align(onionCard, LV_ALIGN_TOP_LEFT, 55, 135);
    tomatoCard = panel(root, 310, 265); lv_obj_align(tomatoCard, LV_ALIGN_TOP_RIGHT, -55, 135);
    vegetable_art(onionCard, true); vegetable_art(tomatoCard, false);
    lv_obj_t *ot = label(onionCard, "ONION", INK, &lv_font_montserrat_40); lv_obj_align(ot, LV_ALIGN_TOP_MID, 0, 137);
    lv_obj_t *tt = label(tomatoCard, "TOMATO", INK, &lv_font_montserrat_40); lv_obj_align(tt, LV_ALIGN_TOP_MID, 0, 137);
    lv_obj_t *ob = button(onionCard, onion ? "SELECTED  ✓" : "TAP TO SELECT", 205, 42, onion ? GREEN : BTN_NEUTRAL, onion ? 0xFFFFFF : BTN_TEXT); lv_obj_align(ob, LV_ALIGN_BOTTOM_MID, 0, -3); lv_obj_add_event_cb(ob, choose_onion, LV_EVENT_CLICKED, NULL);
    lv_obj_t *tb = button(tomatoCard, tomato ? "SELECTED  ✓" : "TAP TO SELECT", 205, 42, tomato ? GREEN : BTN_NEUTRAL, tomato ? 0xFFFFFF : BTN_TEXT); lv_obj_align(tb, LV_ALIGN_BOTTOM_MID, 0, -3); lv_obj_add_event_cb(tb, choose_tomato, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = button(root, "NEXT  >", 165, 46, ORANGE); lv_obj_align(next, LV_ALIGN_BOTTOM_RIGHT, -35, -18); lv_obj_add_event_cb(next, go_servings, LV_EVENT_CLICKED, NULL);
    fade_in();
}

static void show_servings() {
    currentPage = 2;
    clear_screen(); header("DISPENSE / 02", "Choose the serving quantity");
    lv_obj_t *back = button(root, "< BACK", 95, 38, BTN_NEUTRAL, BTN_TEXT); lv_obj_align(back, LV_ALIGN_TOP_LEFT, 30, 95); lv_obj_add_event_cb(back, [](lv_event_t *) { show_modules(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *summary = label(root, onion && tomato ? "ONION + TOMATO SELECTED" : (onion ? "ONION SELECTED" : "TOMATO SELECTED"), MUTED); lv_obj_align(summary, LV_ALIGN_TOP_MID, 0, 118);
    oneCard = panel(root, 260, 175); lv_obj_align(oneCard, LV_ALIGN_TOP_LEFT, 110, 175);
    twoCard = panel(root, 260, 175); lv_obj_align(twoCard, LV_ALIGN_TOP_RIGHT, -110, 175);
    lv_obj_t *one = label(oneCard, "1", ORANGE, &lv_font_montserrat_40); lv_obj_align(one, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t *oneTxt = label(oneCard, "SERVING", INK); lv_obj_align(oneTxt, LV_ALIGN_TOP_MID, 0, 78);
    lv_obj_t *oneBtn = button(oneCard, servings == 1 ? "SELECTED  ✓" : "CHOOSE", 190, 40, servings == 1 ? GREEN : BTN_NEUTRAL, servings == 1 ? 0xFFFFFF : BTN_TEXT); lv_obj_align(oneBtn, LV_ALIGN_BOTTOM_MID, 0, -2); lv_obj_add_event_cb(oneBtn, choose_one, LV_EVENT_CLICKED, NULL);
    lv_obj_t *two = label(twoCard, "2", ORANGE, &lv_font_montserrat_40); lv_obj_align(two, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t *twoTxt = label(twoCard, "SERVINGS", INK); lv_obj_align(twoTxt, LV_ALIGN_TOP_MID, 0, 78);
    lv_obj_t *twoBtn = button(twoCard, servings == 2 ? "SELECTED  ✓" : "CHOOSE", 190, 40, servings == 2 ? GREEN : BTN_NEUTRAL, servings == 2 ? 0xFFFFFF : BTN_TEXT); lv_obj_align(twoBtn, LV_ALIGN_BOTTOM_MID, 0, -2); lv_obj_add_event_cb(twoBtn, choose_two, LV_EVENT_CLICKED, NULL);
    lv_obj_t *submitBtn = button(root, "CONFIRM  >", 180, 46, ORANGE); lv_obj_align(submitBtn, LV_ALIGN_BOTTOM_RIGHT, -35, -15); lv_obj_add_event_cb(submitBtn, submit, LV_EVENT_CLICKED, NULL);
    fade_in();
}

static void start_dispensing() {
    Serial.println("Confirm pressed");

    if (!onion && !tomato) {
        Serial.println("Select an ingredient first.");
        return;
    }

    if (!beginDispenseRequest(onion, tomato, servings)) {
        Serial.println("Could not start dispensing.");
        return;
    }

    liveDispenseActive = true;
    liveDispenseFailed = false;

    if (dispenseTimer) {
        lv_timer_delete(dispenseTimer);
        dispenseTimer = nullptr;
    }

    currentPage = 5;
    clear_screen();
    header("DISPENSING", "Waiting for GravyMaker controller...");
    lv_obj_t *msg = label(root, "Connecting to machine...", INK,
                          &lv_font_montserrat_40);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 20);
}

static void show_live_ingredient(bool isOnion, bool completed) {
    currentPage = 5;
    clear_screen();

    const char *ingredient = isOnion ? "ONION" : "TOMATO";
    header("DISPENSING", completed
        ? "Ingredient sequence completed"
        : "GravyMaker is operating");

    lv_obj_t *machine = lv_obj_create(root);
    lv_obj_remove_style_all(machine);
    lv_obj_set_size(machine, 240, 180);
    lv_obj_set_style_bg_opa(machine, LV_OPA_TRANSP, 0);
    lv_obj_align(machine, LV_ALIGN_TOP_MID, 0, 100);

    lv_obj_t *chute = lv_obj_create(machine);
    lv_obj_remove_style_all(chute);
    lv_obj_set_size(chute, 100, 22);
    lv_obj_set_style_radius(chute, 6, 0);
    lv_obj_set_style_bg_color(chute,
        lv_color_hex(isOnion ? ORANGE : RED), 0);
    lv_obj_set_style_bg_opa(chute, LV_OPA_COVER, 0);
    lv_obj_align(chute, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *bowl = lv_obj_create(machine);
    lv_obj_remove_style_all(bowl);
    lv_obj_set_size(bowl, 150, 44);
    lv_obj_set_style_radius(bowl, 10, 0);
    lv_obj_set_style_bg_color(bowl, lv_color_hex(PALE), 0);
    lv_obj_set_style_bg_opa(bowl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bowl,
        lv_color_hex(isOnion ? ORANGE : RED), 0);
    lv_obj_set_style_border_width(bowl, 3, 0);
    lv_obj_align(bowl, LV_ALIGN_BOTTOM_MID, 0, -8);

    lv_obj_t *img = lv_image_create(machine);
    lv_image_set_src(img, isOnion ? &gravy_onion_image
                                  : &gravy_tomato_image);
    lv_image_set_scale(img, 90);

    if (completed) {
        lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, -18);
    } else {
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 35);

        lv_anim_t drop;
        lv_anim_init(&drop);
        lv_anim_set_var(&drop, img);
        lv_anim_set_values(&drop, 0, 55);
        lv_anim_set_duration(&drop, 700);
        lv_anim_set_playback_duration(&drop, 350);
        lv_anim_set_repeat_count(&drop, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&drop,
            [](void *obj, int32_t v) {
                lv_obj_set_style_translate_y(
                    static_cast<lv_obj_t *>(obj), v, 0);
            });
        lv_anim_start(&drop);
    }

    lv_obj_t *title = label(root,
        completed ? "INGREDIENT COMPLETE" :
                    (isOnion ? "DISPENSING ONION..." :
                               "DISPENSING TOMATO..."),
        INK, &lv_font_montserrat_40);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 300);

    lv_obj_t *detail = label(root, ingredient, MUTED);
    lv_obj_align(detail, LV_ALIGN_TOP_MID, 0, 355);

    if (completed) {
        lv_obj_t *check = label(root, LV_SYMBOL_OK, GREEN,
                                &lv_font_montserrat_40);
        lv_obj_align(check, LV_ALIGN_TOP_MID, 0, 400);
    }
}

inline void handleDispenseStatus(const String &status) {
    Serial.print("Handling status: ");
    Serial.println(status);

    if (status == "SPICECHECK_START") {
        for (int i = 0; i < 12; ++i) {
            spiceLevelsUI[i] = -1;
        }

        show_spice_loading();
    }
    else if (status.startsWith("SPICE_LEVEL|")) {
        int firstBar = status.indexOf('|');
        int secondBar = status.indexOf('|', firstBar + 1);

        if (firstBar > 0 && secondBar > firstBar) {
            int compartment =
                status.substring(firstBar + 1, secondBar).toInt();
            int level =
                status.substring(secondBar + 1).toInt();

            if (compartment >= 1 && compartment <= 12 &&
                level >= -1 && level <= 1) {
                spiceLevelsUI[compartment - 1] = level;

                Serial.printf("Compartment %d = %d\n",
                              compartment, level);
            }
        }
    }
    else if (status == "SPICECHECK_DONE") {
        show_spices();
    }
    else if (status == "ONION_START") {
        show_live_ingredient(true, false);
    }
    else if (status == "ONION_DONE") {
        show_live_ingredient(true, true);
    }
    else if (status == "TOMATO_START") {
        show_live_ingredient(false, false);
    }
    else if (status == "TOMATO_DONE") {
        show_live_ingredient(false, true);
    }
    else if (status == "DISPENSE_DONE") {
        liveDispenseActive = false;
        liveDispenseFailed = false;
        show_complete();
    }
    else if (status.startsWith("ERROR|")) {
        liveDispenseActive = false;
        liveDispenseFailed = true;

        currentPage = 5;
        clear_screen();
        header("OPERATION ERROR",
               "GravyMaker could not complete the request");

        lv_obj_t *msg = label(
            root, status.c_str(), RED, &lv_font_montserrat_40);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *back = button(
            root, "RETURN HOME", 220, 50, ORANGE);
        lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -25);
        lv_obj_add_event_cb(
            back, back_to_home, LV_EVENT_CLICKED, NULL);
    }
}

static void advance_dispense_stage(lv_timer_t *timer) {
    if (timer) lv_timer_delete(timer);
    dispenseTimer = nullptr;
    if (dispenseStage < dispenseOrderCount * 2) {
        ++dispenseStage;
        show_dispense_stage();
    } else {
        dispenseFinished = true;
        show_complete();
    }
}

static void show_dispense_stage() {
    currentPage = 5;
    clear_screen();
    if (dispenseFinished || dispenseStage >= dispenseOrderCount * 2) {
        dispenseFinished = true;
        show_complete();
        return;
    }
    const bool isOnion = dispenseOrderIsOnion[dispenseStage / 2];
    const bool isDispensed = (dispenseStage % 2) == 1;
    uint8_t itemNumber = 1;
    for (int i = 0; i < (dispenseStage / 2); ++i) {
        if (dispenseOrderIsOnion[i] == isOnion) ++itemNumber;
    }
    const uint8_t itemTotal = isOnion ? (onion ? servings : 0) : (tomato ? servings : 0);
    char section[32];
    snprintf(section, sizeof(section), "%s %u / %u", isOnion ? "ONION" : "TOMATO", itemNumber, itemTotal);
    header("DISPENSING", "Please wait while your selection is processed");

    // Relevant dispensing animation: a chute drops the selected ingredient into a bowl.
    // Large, high-contrast shapes and a moving ingredient make the animation obvious on the 7-inch panel.
    lv_obj_t *machine = lv_obj_create(root); lv_obj_remove_style_all(machine);
    lv_obj_set_size(machine, 250, 170); lv_obj_align(machine, LV_ALIGN_TOP_MID, 0, 105);
    lv_obj_set_style_bg_opa(machine, LV_OPA_TRANSP, 0);

    lv_obj_t *topBox = lv_obj_create(machine); lv_obj_remove_style_all(topBox);
    lv_obj_set_size(topBox, 86, 24); lv_obj_set_style_radius(topBox, 6, 0);
    lv_obj_set_style_bg_color(topBox, lv_color_hex(isOnion ? ORANGE : RED), 0);
    lv_obj_set_style_bg_opa(topBox, LV_OPA_COVER, 0); lv_obj_align(topBox, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t *chute = lv_obj_create(machine); lv_obj_remove_style_all(chute);
    lv_obj_set_size(chute, 48, 32); lv_obj_set_style_radius(chute, 4, 0);
    lv_obj_set_style_bg_color(chute, lv_color_hex(BORDER), 0); lv_obj_set_style_bg_opa(chute, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chute, lv_color_hex(MUTED), 0); lv_obj_set_style_border_width(chute, 2, 0);
    lv_obj_align(chute, LV_ALIGN_TOP_MID, 0, 22);

    lv_obj_t *bowl = lv_obj_create(machine); lv_obj_remove_style_all(bowl);
    lv_obj_set_size(bowl, 142, 43); lv_obj_set_style_radius(bowl, 8, 0);
    lv_obj_set_style_bg_color(bowl, lv_color_hex(PALE), 0); lv_obj_set_style_bg_opa(bowl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bowl, lv_color_hex(isOnion ? ORANGE : RED), 0); lv_obj_set_style_border_width(bowl, 3, 0);
    lv_obj_align(bowl, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_t *bowlInner = lv_obj_create(bowl); lv_obj_remove_style_all(bowlInner);
    lv_obj_set_size(bowlInner, 100, 8); lv_obj_set_style_radius(bowlInner, 4, 0);
    lv_obj_set_style_bg_color(bowlInner, lv_color_hex(isOnion ? ORANGE : RED), 0); lv_obj_set_style_bg_opa(bowlInner, LV_OPA_60, 0);
    lv_obj_align(bowlInner, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *img = lv_image_create(machine);
    lv_image_set_src(img, isOnion ? &gravy_onion_image : &gravy_tomato_image);
    lv_image_set_scale(img, 90);
    if (isDispensed) {
        lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, -18);
        lv_obj_set_style_opa(img, LV_OPA_COVER, 0);
        lv_obj_t *itemCheck = label(machine, LV_SYMBOL_OK, GREEN, &lv_font_montserrat_40);
        lv_obj_align(itemCheck, LV_ALIGN_RIGHT_MID, -12, -18);
        lv_anim_t checkPop; lv_anim_init(&checkPop); lv_anim_set_var(&checkPop, itemCheck);
        lv_anim_set_values(&checkPop, 18, 0); lv_anim_set_duration(&checkPop, 420);
        lv_anim_set_exec_cb(&checkPop, [](void *v, int32_t n) { lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(v), n, 0); }); lv_anim_start(&checkPop);
    } else {
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 50);
        lv_anim_t dropAnim; lv_anim_init(&dropAnim); lv_anim_set_var(&dropAnim, img);
        lv_anim_set_values(&dropAnim, 0, 62); lv_anim_set_duration(&dropAnim, 850);
        lv_anim_set_playback_duration(&dropAnim, 180); lv_anim_set_repeat_count(&dropAnim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&dropAnim, [](void *v, int32_t n) { lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(v), n, 0); });
        lv_anim_start(&dropAnim);
    }

    lv_obj_t *title = label(root, isDispensed ? (isOnion ? "ONION DISPENSED" : "TOMATO DISPENSED") : (isOnion ? "DISPENSING ONION..." : "DISPENSING TOMATO..."), INK, &lv_font_montserrat_40);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 292);
    char detail[80];
    snprintf(detail, sizeof(detail), "%s %u of %u", isOnion ? "Onion" : "Tomato", itemNumber, itemTotal);
    lv_obj_t *desc = label(root, detail, MUTED); lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 344);

    lv_obj_t *bar = lv_bar_create(root); lv_obj_set_size(bar, 360, 16); lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 380);
    lv_obj_set_style_radius(bar, 8, LV_PART_MAIN); lv_obj_set_style_bg_color(bar, lv_color_hex(BORDER), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR); lv_obj_set_style_bg_color(bar, lv_color_hex(isOnion ? ORANGE : RED), LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 100); lv_bar_set_value(bar, isDispensed ? 100 : 0, LV_ANIM_OFF);
    if (!isDispensed) {
        lv_anim_t progress; lv_anim_init(&progress); lv_anim_set_var(&progress, bar);
        lv_anim_set_values(&progress, 0, 100); lv_anim_set_duration(&progress, 2050);
        lv_anim_set_exec_cb(&progress, [](void *v, int32_t n) { lv_bar_set_value(static_cast<lv_obj_t *>(v), n, LV_ANIM_OFF); });
        lv_anim_start(&progress);
    }

    // Each item gets a dispensing stage and a dispensed confirmation stage.
    // The delay is intentionally simulated for UI testing only.
    dispenseTimer = lv_timer_create(advance_dispense_stage, isDispensed ? 1100 : 2200, nullptr);
}

static void show_complete() {
    currentPage = 3;
    clear_screen(); header("COMPLETE", "Your dispensing sequence has finished");
    lv_obj_t *badge = lv_obj_create(root); lv_obj_remove_style_all(badge); lv_obj_set_size(badge, 118, 118); lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0xDDF7E8), 0); lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(GREEN), 0); lv_obj_set_style_border_width(badge, 4, 0); lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_t *check = label(badge, LV_SYMBOL_OK, 0x16834A, &lv_font_montserrat_40); lv_obj_center(check);
    lv_obj_set_style_opa(check, LV_OPA_TRANSP, 0);
    lv_anim_t checkReveal; lv_anim_init(&checkReveal); lv_anim_set_var(&checkReveal, check); lv_anim_set_values(&checkReveal, LV_OPA_TRANSP, LV_OPA_COVER); lv_anim_set_duration(&checkReveal, 500);
    lv_anim_set_exec_cb(&checkReveal, [](void *v, int32_t n) { lv_obj_set_style_opa(static_cast<lv_obj_t *>(v), static_cast<lv_opa_t>(n), 0); }); lv_anim_start(&checkReveal);
    lv_anim_t pulse; lv_anim_init(&pulse); lv_anim_set_var(&pulse, badge); lv_anim_set_values(&pulse, 108, 118); lv_anim_set_duration(&pulse, 420); lv_anim_set_playback_duration(&pulse, 420); lv_anim_set_repeat_count(&pulse, 3);
    lv_anim_set_exec_cb(&pulse, [](void *v, int32_t n) { lv_obj_set_size(static_cast<lv_obj_t *>(v), n, n); }); lv_anim_start(&pulse);
    lv_obj_t *title = label(root, "DISPENSE SUCCESSFUL", INK, &lv_font_montserrat_40); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 250);
    char msg[100];
    snprintf(msg, sizeof(msg), "%s%s%s  |  %u %s", onion ? "Onion" : "", onion && tomato ? " + " : "", tomato ? "Tomato" : "", servings, servings == 1 ? "serving" : "servings");
    lv_obj_t *desc = label(root, msg, MUTED); lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 305);
    lv_obj_t *home = button(root, "RETURN HOME", 230, 54, ORANGE); lv_obj_align(home, LV_ALIGN_BOTTOM_MID, 0, -28); lv_obj_add_event_cb(home, [](lv_event_t *) { if (dispenseTimer) { lv_timer_delete(dispenseTimer); dispenseTimer = nullptr; } onion = false; tomato = false; servings = 1; dispenseFinished = false; show_home(); }, LV_EVENT_CLICKED, NULL);
    fade_in();
}

static void finish_spice_loading(lv_timer_t *timer) {
    if (timer) lv_timer_delete(timer);
    spiceTimer = nullptr;
    show_spices();
}

static void show_spice_loading() {
    currentPage = 6;
    clear_screen();
    header("SPICE CHECK", "Please wait while we check all 12 compartments");

    // High-contrast spice jar and falling grains; the progress bar visibly fills for 2.8 seconds.
    lv_obj_t *scene = lv_obj_create(root); lv_obj_remove_style_all(scene);
    lv_obj_set_size(scene, 240, 210); lv_obj_align(scene, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_bg_opa(scene, LV_OPA_TRANSP, 0);
    lv_obj_t *jar = lv_obj_create(scene); lv_obj_remove_style_all(jar); lv_obj_set_size(jar, 104, 116);
    lv_obj_set_style_radius(jar, 20, 0); lv_obj_set_style_bg_color(jar, lv_color_hex(PALEG), 0); lv_obj_set_style_bg_opa(jar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(jar, lv_color_hex(GREEN), 0); lv_obj_set_style_border_width(jar, 4, 0); lv_obj_align(jar, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_t *fill = lv_obj_create(jar); lv_obj_remove_style_all(fill); lv_obj_set_size(fill, 68, 40);
    lv_obj_set_style_radius(fill, 10, 0); lv_obj_set_style_bg_color(fill, lv_color_hex(0xD99A18), 0); lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0); lv_obj_align(fill, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_t *lid = lv_obj_create(scene); lv_obj_remove_style_all(lid); lv_obj_set_size(lid, 68, 16);
    lv_obj_set_style_radius(lid, 5, 0); lv_obj_set_style_bg_color(lid, lv_color_hex(ORANGE), 0); lv_obj_set_style_bg_opa(lid, LV_OPA_COVER, 0); lv_obj_align_to(lid, jar, LV_ALIGN_OUT_TOP_MID, 0, 3);

    const uint32_t grains[5] = {0xD99A18, 0xE97735, 0x55C995, 0xC58B36, 0xD99A18};
    for (int i=0; i<5; ++i) {
        lv_obj_t *grain = lv_obj_create(scene); lv_obj_remove_style_all(grain);
        lv_obj_set_size(grain, (i%2)?9:12, (i%2)?9:12); lv_obj_set_style_radius(grain, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(grain, lv_color_hex(grains[i]), 0); lv_obj_set_style_bg_opa(grain, LV_OPA_COVER, 0);
        lv_obj_align(grain, LV_ALIGN_TOP_MID, (i-2)*27, 5+(i%2)*12);
        lv_anim_t fall; lv_anim_init(&fall); lv_anim_set_var(&fall, grain); lv_anim_set_values(&fall, 0, 112);
        lv_anim_set_duration(&fall, 650+(i*110)); lv_anim_set_playback_duration(&fall, 100);
        lv_anim_set_repeat_count(&fall, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&fall, [](void *v, int32_t n) { lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(v), n, 0); }); lv_anim_start(&fall);
    }
    lv_obj_t *title = label(root, "MEASURING SPICE LEVELS...", INK, &lv_font_montserrat_40);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 316);
    lv_obj_t *desc = label(root, "Scanning compartment levels", MUTED); lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 360);
    lv_obj_t *bar = lv_bar_create(root); lv_obj_set_size(bar, 390, 17); lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 395);
    lv_obj_set_style_radius(bar, 9, LV_PART_MAIN); lv_obj_set_style_bg_color(bar, lv_color_hex(BORDER), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 9, LV_PART_INDICATOR); lv_obj_set_style_bg_color(bar, lv_color_hex(GREEN), LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 100); lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_anim_t progress; lv_anim_init(&progress); lv_anim_set_var(&progress, bar); lv_anim_set_values(&progress, 0, 90);
lv_anim_set_duration(&progress, 12000);
lv_anim_set_repeat_count(&progress, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&progress, [](void *v, int32_t n) { lv_bar_set_value(static_cast<lv_obj_t *>(v), n, LV_ANIM_OFF); }); lv_anim_start(&progress);
    fade_in();
    
}

static const char *status_name(int level) {
    switch (level) {
        case -1: return "INSUFFICIENT";
        case  0: return "ENOUGH FOR 2";
        case  1: return "FULL";
        default: return "NOT MEASURED";
    }
}

static uint32_t status_color(int level) {
    switch (level) {
        case -1: return RED;
        case  0: return 0xD99A18;
        case  1: return GREEN;
        default: return MUTED;
    }
}
static void animate_spice(lv_obj_t *obj) {
    lv_anim_t a; lv_anim_init(&a); lv_anim_set_var(&a, obj); lv_anim_set_values(&a, 0, 7); lv_anim_set_duration(&a, 450); lv_anim_set_playback_duration(&a, 450); lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, [](void *v, int32_t n) {
        lv_obj_t *dot = static_cast<lv_obj_t *>(v);
        lv_obj_set_style_translate_y(dot, n, 0);
        lv_obj_set_style_shadow_width(dot, 4 + n, 0);
        lv_obj_set_style_shadow_opa(dot, LV_OPA_40, 0);
    }); lv_anim_start(&a);
}
static void show_spices() {
    currentPage = 4;
    clear_screen();

    header("SPICE CHECK", "Live results from all 12 compartments");

    lv_obj_t *back = button(root, "< BACK", 95, 38,
                            BTN_NEUTRAL, BTN_TEXT);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 30, 95);
    lv_obj_add_event_cb(back, back_to_home,
                        LV_EVENT_CLICKED, NULL);

    for (int i = 0; i < 12; ++i) {
        int col = i % 4;
        int row = i / 4;
        int level = spiceLevelsUI[i];

        lv_obj_t *p = panel(root, 170, 88);
        lv_obj_set_style_pad_all(p, 9, 0);
        lv_obj_align(p, LV_ALIGN_TOP_LEFT,
                     32 + col * 190, 145 + row * 96);

        char name[24];
        snprintf(name, sizeof(name), "COMPARTMENT %02d", i + 1);

        lv_obj_t *n = label(p, name, INK);
        lv_obj_align(n, LV_ALIGN_TOP_LEFT, 2, 0);

        lv_obj_t *well = lv_obj_create(p);
        lv_obj_remove_style_all(well);
        lv_obj_set_size(well, 25, 25);
        lv_obj_set_style_radius(well, 8, 0);
        lv_obj_set_style_bg_color(
            well, lv_color_hex(status_color(level)), 0);
        lv_obj_set_style_bg_opa(well, LV_OPA_COVER, 0);
        lv_obj_align(well, LV_ALIGN_BOTTOM_LEFT, 2, -3);

        lv_obj_t *st = label(
            p, status_name(level), status_color(level));
        lv_obj_align(st, LV_ALIGN_BOTTOM_LEFT, 36, -8);
    }

    lv_obj_t *legend = label(
        root,
        "RED  Insufficient   AMBER  Enough for 2 servings   GREEN  Full",
        MUTED);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -12);

    fade_in();
}
inline void init() {
    apply_theme();
    onion = false; tomato = false; servings = 1;
    show_home();
}

} // namespace gravyui
