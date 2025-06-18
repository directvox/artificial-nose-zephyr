#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/ring_buffer.h>

#include "zbus_messages.h"
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

ZBUS_CHAN_DECLARE(inference_result_chan);

// Définitions pour l'écran (maintenues, mais assurez-vous qu'elles sont définies ailleurs si DISPLAY_NODE n'est pas utilisé)
#define DISPLAY_NODE DT_CHOSEN(zephyr_display)
#define DISPLAY_WIDTH  DT_PROP(DISPLAY_NODE, width)
#define DISPLAY_HEIGHT DT_PROP(DISPLAY_NODE, height)
#define IS_LARGE_SCREEN (DISPLAY_WIDTH > 240)

static lv_obj_t *chart;
static lv_chart_series_t *series[4];
static lv_obj_t *inference_result_label;

#if IS_LARGE_SCREEN
// Déclaration du nouvel objet lv_arc pour l'indicateur de confiance
static lv_obj_t *inference_result_confidence_arc;
#endif

static lv_palette_t palette_colors[4] = {
	LV_PALETTE_RED,
	LV_PALETTE_GREEN,
	LV_PALETTE_BLUE,
	LV_PALETTE_YELLOW,
};

static lv_timer_t *sensor_timer;

/**
 * Zbus callback for inference results
 *
 * Called every time a new message is published to the inference result channel
 */
static void inference_cb(const struct zbus_channel *chan)
{
	const struct inference_result_msg *msg = zbus_chan_const_msg(chan);

	lv_label_set_text(inference_result_label, msg->label);
#if IS_LARGE_SCREEN
	// Mise à jour de la valeur de l'arc
	// L'arc s'attend à une valeur dans sa plage définie (0-100 ici)
	lv_arc_set_value(inference_result_confidence_arc, (int)(msg->confidence * 100));
#endif
}

ZBUS_LISTENER_DEFINE(inference_ui_listener, inference_cb);

extern struct k_sem sensor_data_ringbuf_sem;
extern struct ring_buf sensor_data_ringbuf;

/**
 * LVLGL timer handler
 * Gets sensor data directly from ring buffer and append it to the chart
 */
static void sensor_timer_cb(lv_timer_t *timer)
{
	uint32_t buffer[4];

	k_sem_take(&sensor_data_ringbuf_sem, K_FOREVER);
	int ret = ring_buf_peek(&sensor_data_ringbuf, (uint8_t*)buffer, 4 * sizeof(uint32_t));
	k_sem_give(&sensor_data_ringbuf_sem);

	if (ret < 4) {
		return;
	}

	for (int i = 0; i < 4; i++) {
		lv_chart_set_next_value(chart, series[i], buffer[i]);
	}
}

void create_sensor_chart(lv_obj_t *parent)
{
	/* Add zbus observer to be notified of new inference results */
	zbus_chan_add_obs(&inference_result_chan, &inference_ui_listener, K_MSEC(200));

	chart = lv_chart_create(parent);
	// Utilisation de LV_PCT(100) pour une taille relative si possible, ou ajuster les dimensions
    // en fonction de la résolution de l'écran si LV_HOR_RES et LV_VER_RES sont des macros de taille absolue.
	lv_obj_set_size(chart, LV_HOR_RES, LV_VER_RES);
	lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(chart, 5, 8);
	lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 800);
	lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);

	for (int i = 0; i < 4; i++) {
		series[i] = lv_chart_add_series(chart, lv_palette_main(palette_colors[i]),
						LV_CHART_AXIS_PRIMARY_Y);
	}

	lv_chart_set_point_count(chart, 400);

	#if IS_LARGE_SCREEN
	// Création de l'Arc à la place du Meter
	inference_result_confidence_arc = lv_arc_create(lv_scr_act());
	lv_obj_center(inference_result_confidence_arc);
	lv_obj_set_size(inference_result_confidence_arc, 110, 110);

	// Le style de police sur LV_PART_TICKS n'est pas applicable à lv_arc.
    // Si des textes doivent être affichés sur l'arc, un lv_label séparé est nécessaire.
	// lv_obj_set_style_text_font(inference_result_confidence_arc, &lv_font_montserrat_8, LV_PART_TICKS);

	// Styles de l'arc
	lv_obj_set_style_bg_opa(inference_result_confidence_arc, LV_OPA_80, LV_PART_MAIN); // Arrière-plan de l'arc
    lv_obj_set_style_arc_width(inference_result_confidence_arc, 10, LV_PART_MAIN); // Largeur de l'arc

    // Configuration de l'arc pour agir comme un indicateur de 0 à 100
    // Rotation pour que 0 soit en bas à gauche et 100 en bas à droite (un arc de 270 degrés)
    lv_arc_set_rotation(inference_result_confidence_arc, 135); // Commence à 135 degrés (bas-gauche)
    lv_arc_set_bg_angles(inference_result_confidence_arc, 0, 270); // Angle total de l'arc de fond (0-270 degrés pour un arc complet)
    lv_arc_set_range(inference_result_confidence_arc, 0, 100); // Plage de valeurs de l'arc

    // Style de l'indicateur de l'arc (la partie qui bouge)
    lv_obj_set_style_arc_color(inference_result_confidence_arc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(inference_result_confidence_arc, 10, LV_PART_INDICATOR); // Largeur de l'indicateur
    lv_obj_set_style_line_rounded(inference_result_confidence_arc, true, LV_PART_INDICATOR); // Bords arrondis pour l'indicateur

	// Supprime les configurations spécifiques au meter
	// lv_meter_scale_t *scale = lv_meter_add_scale(inference_result_confidence_meter);
	// lv_meter_set_scale_ticks(...);
	// lv_meter_set_scale_major_ticks(...);
	// inference_result_confidence_indic = lv_meter_add_needle_line(...);
	#endif

	inference_result_label = lv_label_create(lv_scr_act());
#if IS_LARGE_SCREEN
	lv_obj_align(inference_result_label, LV_ALIGN_CENTER, 0, 80); // Positionner le label en dessous de l'arc
#else
	lv_obj_align(inference_result_label, LV_ALIGN_CENTER, 0, 0);
#endif

	sensor_timer = lv_timer_create(sensor_timer_cb, 50, NULL);
}

#ifdef __cplusplus
}
#endif