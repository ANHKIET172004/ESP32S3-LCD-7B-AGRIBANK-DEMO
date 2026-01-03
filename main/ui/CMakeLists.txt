SET(SOURCES ui_Screen1.c
    ui_Screen2.c
    ui_Screen3.c
    ui_Screen4.c
    ui_Screen5.c
    ui_Screen6.c
    ui_wifi_Screen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_rat_hai_long_165x165_png.c
    ui_img_hai_long_165x165_png.c
    ui_img_binh_thuong_165x165_png.c
    ui_img_that_vong_165x165_png.c
    ui_img_rat_khong_hai_long_165x165_png.c
    ui_img_27268529.c
    ui_img_167099737.c
    ui_img_star_grey_png.c
    ui_img_1105461608.c
    ui_img_1749747114.c
    ui_img_656053819.c
    ui_img_608776254.c

    images/ui_img_wifi_1_png.c
    images/ui_img_wifi_2_png.c
    images/ui_img_wifi_3_png.c
    images/ui_img_wifi_4_png.c

    images/ui_img_log_on_png.c
    images/ui_img_eye_png.c

    images/wifi_1_100x100.c
    images/wifi_1_20x20.c
    images/wifi_2_20x20.c
    images/wifi_3_20x20.c
    images/wifi_4_20x20.c
    images/ui_img_nowifi_50x50_png.c
    images/ui_img_wifiicon2_50x50_png.c
    ui_img_776640815.c
    images/ui_img_785059581.c
    images/ui_img_1279478591.c
    images/ui_img_wifi1_tran_png.c
    images/ui_img_nowifi3_50x50_trans_png.c
    images/ui_img_red_circle_70x70_png.c
    images/ui_img_green_circle_2_70x70_png.c
    images/ui_img_red_circle_20x20_png.c
    images/ui_img_turn_back2_png.c


    
    fonts/ui_font_Chinese_Font.c

    fonts/ui_font_BOLD_VN20.c
    fonts/ui_font_BOLD_VN25.c
    fonts/ui_font_BOLD_VN30.c
    fonts/ui_font_BOLD_VN35.c
    fonts/ui_font_BOLD_VN80.c)

add_library(ui ${SOURCES})
