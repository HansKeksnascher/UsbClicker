#include "../include/ctx.h"
#include "../include/video.h"

static unsigned long long money = 0;
static char buffer[1024];
static font_t *font_proggy_clean;
static int money_timer;
static int news_timer;

static int cam_timer;
static int cam_index;
static sprite_t *sprite_cam[3];

typedef struct {
    char *name;
    int cost;
    int income;
    int count;
    sprite_t *sprite;
} upgrade_t;

static upgrade_t upgrades[] = {
        {
                .name = "Schaltplan scrollen",
                .cost = 10,
                .income = 1,
                .count = 0
        },
        {
                .name = "Xbox Controller",
                .cost = 100,
                .income = 10,
                .count = 0
        },
        {
                .name = "GameStop Nebenjob",
                .cost = 1000,
                .income = 100,
                .count = 0
        },
        {
                .name = "FRITZ!Box",
                .cost = 10000,
                .income = 1000,
                .count = 0
        },
        {
                .name = "Home automation",
                .cost = 100000,
                .income = 10000,
                .count = 0
        },
        {
                .name = "Copy & Paste",
                .cost = 1000000,
                .income = 100000,
                .count = 0
        },
        {
                .name = "USB Type-D",
                .cost = 10000000,
                .income = 1000000,
                .count = 0
        }
};

static char *messages[] = {
        "Kein Ende der Gentwoche in Sicht",
        "Doktor Schnauf als bester Wirtschaftsinformatiker 2017 ausgezeichnet",
        "Starke Proteste gegen Inder auf YouTube",
        "Susi vermisst",
        "Der mit den Schuhen hat neue Schuhe",
        "Gent stellt neues AAA-Game Snake Reloaded vor",
        "Daniel trotz Schluessel nie im Labor",
        "Alles ist doch nicht so trivial",
        "Alex vergibt heute den Rechercheorden des Jahres",
        "Daniel stellt neues USB auf der Gamescom vor",
        "Tag 521: USB wurde um 1 Grad gedreht. Recherche weiterhin erfolglos",
        "Austellung Kristallzucht jetzt im GG Labor",
        "Gent mit goldenem PoE-Tradeorden ausgezeichnet",
        "Wissenschaftlich bewiesen: Daniel faul",
        "Akte Gent geleakt",
        "Monster ausverkauft. Ist Doktor Schnauf schuld?",
        "Drei Neue Susi-Spiele dieses Jahr angekuendigt",
        "Neue Modemarke Tutorial by Daniel gestartet",
        "Spuerst du das USB heut Nacht",
        "Glasmeister Alex veroeffentlicht neuen Hit Summertime Recherche",
        "Wissenschaftliche Fachkraft hat keine Ahnung von Wissenschaft",
        "Gent zum CEO des Jahres nominiert"
};

static char *news_message = NULL;

static void on_mouse_click(vec2_t pos) {
    for (int i = 0; i < ARRAY_LENGTH(upgrades); i++) {
        vec4_t rect = vec4_new(714, 40 + i * 72, 200, 32);
        if (vec4_point_inside(rect, pos)) {
            if (upgrades[i].cost <= money) {
                upgrades[i].count++;
                money -= upgrades[i].cost;
            }
            return;
        }
    }
    money++;
}

static void sketch_init() {
    font_proggy_clean = font_load("asset/font/proggy_clean.fnt", "asset/font/proggy_clean.png");
    sprite_cam[0] = sprite_load("asset/sprite/cam0.png");
    sprite_cam[1] = sprite_load("asset/sprite/cam1.png");
    sprite_cam[2] = sprite_load("asset/sprite/cam2.png");
    upgrades[0].sprite = sprite_load("asset/sprite/icon_wiring_plan.png");
    upgrades[1].sprite = sprite_load("asset/sprite/icon_xbox_controller.png");
    upgrades[2].sprite = sprite_load("asset/sprite/icon_gamestop.png");
    upgrades[3].sprite = sprite_load("asset/sprite/icon_fritzbox.png");
    upgrades[4].sprite = sprite_load("asset/sprite/icon_home_automation.png");
    news_message = messages[rand() % ARRAY_LENGTH(messages)];
    ctx_hook_mouse(on_mouse_click);
}

static void sketch_tick() {
    money_timer++;
    news_timer++;
    cam_timer++;
    if (money_timer >= 60) {
        for (int i = 0; i < ARRAY_LENGTH(upgrades); i++) {
            money += upgrades[i].count * upgrades[i].income;
        }
        money_timer = 0;
    }
    if (news_timer >= 500) {
        news_message = messages[rand() % ARRAY_LENGTH(messages)];
        news_timer = 0;
    }
    if (cam_timer >= 20) {
        cam_index = (cam_index + 1) % ARRAY_LENGTH(sprite_cam);
        cam_timer = 0;
    }
}

static void sketch_draw(video_t *video) {
    sprintf(buffer, "C4$h: %llu$", money);
    video_text(video, font_proggy_clean, buffer, 10, 10);
    video_sprite(video, sprite_cam[cam_index], 10, 40);
    video_cfg_mode(video, VIDEO_STROKE);
    for (int i = 0; i < ARRAY_LENGTH(upgrades); i++) {
        if (money >= upgrades[i].cost) {
            video_cfg_color(video, vec4_new(1, 1, 1, 1));
        } else {
            video_cfg_color(video, vec4_new(0.5, 0.5, 0.5, 1));
        }
        sprintf(buffer, "%dx %s", upgrades[i].count, upgrades[i].name);
        if (upgrades[i].sprite) {
            video_sprite(video, upgrades[i].sprite, 630, 10 + i * 72);
        } else {
            video_rectangle(video, 630, 10 + i * 72, 64, 64);
        }
        video_text(video, font_proggy_clean, buffer, 714, 10 + i * 72);
        video_rectangle(video, 714, 40 + i * 72, 200, 32);
        sprintf(buffer, "%d$", upgrades[i].cost);
        video_text(video, font_proggy_clean, buffer, 719, 45 + i * 72);
    }
    video_cfg_color(video, vec4_new(1, 0.5, 0.5, 1));
    if (news_message) {
        sprintf(buffer, "NEWS: %s", news_message);
        video_text(video, font_proggy_clean, buffer, 10, 730);
    }
    video_cfg_color(video, vec4_new(1, 1, 1, 1));
}

static void sketch_shutdown() {
    font_delete(font_proggy_clean);
}

int main(int argc, char **argv) {
    sketch_t sketch = {
            .init = sketch_init,
            .tick = sketch_tick,
            .draw = sketch_draw,
            .shutdown = sketch_shutdown
    };
    return ctx_main(argc, argv, &sketch);
}
