#include "SMeter.h"
#include "UIColorPalette.h" // TFT_COLOR_BACKGROUND definícióhoz

/**
 * Konstruktor.
 * @param tft Referencia a TFT kijelző objektumra.
 * @param smeterX Az S-Meter komponens bal felső sarkának X koordinátája.
 * @param smeterY Az S-Meter komponens bal felső sarkának Y koordinátája.
 */
SMeter::SMeter(TFT_eSPI &tft, uint8_t smeterX, uint8_t smeterY)
    : tft(tft), smeterX(smeterX), smeterY(smeterY), prev_spoint_bars(SMeterConstants::InitialPrevSpoint), prev_rssi_for_text(0xFF), prev_snr_for_text(0xFF) {
    // Inicializáljuk a pozíciós változókat, de a tényleges értékeket a drawSmeterScale-ben számoljuk ki
    rssi_label_x_pos = 0;
    rssi_value_x_pos = 0;
    rssi_value_max_w = 0;
    snr_label_x_pos = 0;
    snr_value_x_pos = 0;
    snr_value_max_w = 0;
    text_y_pos = 0;
    text_h = 0;
}

/**
 * RSSI érték konvertálása S-pont értékre (pixelben).
 * @param rssi Bemenő RSSI érték (0-127 dBuV).
 * @param isFMMode Igaz, ha FM módban vagyunk, hamis AM/SSB/CW esetén.
 * @return A jelerősség pixelben (0-MeterBarMaxPixelValue).
 */
uint8_t SMeter::rssiConverter(uint8_t rssi, bool isFMMode) {
    int spoint_calc; // Ideiglenes változó a számításhoz
    if (isFMMode) {
        // dBuV to S point konverzió FM módra
        if (rssi < 1)
            spoint_calc = 36;
        else if ((rssi >= 1) and (rssi <= 2))
            spoint_calc = 60; // S6
        else if ((rssi > 2) and (rssi <= 8))
            spoint_calc = 84 + (rssi - 2) * 2; // S7
        else if ((rssi > 8) and (rssi <= 14))
            spoint_calc = 96 + (rssi - 8) * 2; // S8
        else if ((rssi > 14) and (rssi <= 24))
            spoint_calc = 108 + (rssi - 14) * 2; // S9
        else if ((rssi > 24) and (rssi <= 34))
            spoint_calc = 124 + (rssi - 24) * 2; // S9 +10dB
        else if ((rssi > 34) and (rssi <= 44))
            spoint_calc = 140 + (rssi - 34) * 2; // S9 +20dB
        else if ((rssi > 44) and (rssi <= 54))
            spoint_calc = 156 + (rssi - 44) * 2; // S9 +30dB
        else if ((rssi > 54) and (rssi <= 64))
            spoint_calc = 172 + (rssi - 54) * 2; // S9 +40dB
        else if ((rssi > 64) and (rssi <= 74))
            spoint_calc = 188 + (rssi - 64) * 2; // S9 +50dB
        else if (rssi > 74 && rssi <= 76)
            spoint_calc = 204; // S9 +60dB
        else if (rssi > 76)
            spoint_calc = SMeterConstants::MeterBarMaxPixelValue; // Max érték
        else
            spoint_calc = 36; // Alapértelmezett minimum FM-re, ha egyik tartomány sem illik
    } else {                  // AM/SSB/CW
        // dBuV to S point konverzió AM/SSB/CW módra
        if ((rssi >= 0) and (rssi <= 1))
            spoint_calc = 12; // S0
        else if ((rssi > 1) and (rssi <= 2))
            spoint_calc = 24; // S1
        else if ((rssi > 2) and (rssi <= 3))
            spoint_calc = 36; // S2
        else if ((rssi > 3) and (rssi <= 4))
            spoint_calc = 48; // S3
        else if ((rssi > 4) and (rssi <= 10))
            spoint_calc = 48 + (rssi - 4) * 2; // S4
        else if ((rssi > 10) and (rssi <= 16))
            spoint_calc = 60 + (rssi - 10) * 2; // S5
        else if ((rssi > 16) and (rssi <= 22))
            spoint_calc = 72 + (rssi - 16) * 2; // S6
        else if ((rssi > 22) and (rssi <= 28))
            spoint_calc = 84 + (rssi - 22) * 2; // S7
        else if ((rssi > 28) and (rssi <= 34))
            spoint_calc = 96 + (rssi - 28) * 2; // S8
        else if ((rssi > 34) and (rssi <= 44))
            spoint_calc = 108 + (rssi - 34) * 2; // S9
        else if ((rssi > 44) and (rssi <= 54))
            spoint_calc = 124 + (rssi - 44) * 2; // S9 +10dB
        else if ((rssi > 54) and (rssi <= 64))
            spoint_calc = 140 + (rssi - 54) * 2; // S9 +20dB
        else if ((rssi > 64) and (rssi <= 74))
            spoint_calc = 156 + (rssi - 64) * 2; // S9 +30dB
        else if ((rssi > 74) and (rssi <= 84))
            spoint_calc = 172 + (rssi - 74) * 2; // S9 +40dB
        else if ((rssi > 84) and (rssi <= 94))
            spoint_calc = 188 + (rssi - 84) * 2; // S9 +50dB
        else if (rssi > 94 && rssi <= 95)
            spoint_calc = 204; // S9 +60dB
        else if (rssi > 95)
            spoint_calc = SMeterConstants::MeterBarMaxPixelValue; // Max érték
        else
            spoint_calc = 0; // Alapértelmezett minimum AM/SSB/CW-re
    }
    // Biztosítjuk, hogy az érték a megengedett tartományban maradjon
    if (spoint_calc < 0)
        spoint_calc = 0;
    if (spoint_calc > SMeterConstants::MeterBarMaxPixelValue)
        spoint_calc = SMeterConstants::MeterBarMaxPixelValue;
    return static_cast<uint8_t>(spoint_calc);
}

/**
 * S-Meter grafikus sávjainak kirajzolása a mért RSSI alapján.
 * @param rssi Aktuális RSSI érték.
 * @param isFMMode Igaz, ha FM módban vagyunk.
 */
void SMeter::drawMeterBars(uint8_t rssi, bool isFMMode) {
    using namespace SMeterConstants;
    uint8_t spoint = rssiConverter(rssi, isFMMode); // Jelerősség pixelben

    if (spoint == prev_spoint_bars)
        return; // Optimalizáció: ne rajzoljunk sávokat feleslegesen
    prev_spoint_bars = spoint;

    int tik = 0;      // 'tik': aktuálisan rajzolt sáv indexe (S0, S1, ..., S9+10dB, ...)
    int met = spoint; // 'met': hátralévő "jelerősség energia" pixelben, amit még ki kell rajzolni

    // Az utolsó színes sáv abszolút X koordinátája a kijelzőn.
    // Kezdetben a piros sáv (S0) elejére mutat. Ha spoint=0, ez marad.
    int end_of_colored_x_abs = smeterX + MeterBarRedStartX;

    // Piros (S0) és narancs (S1-S8) sávok rajzolása
    // Ciklus amíg van 'met' (energia) ÉS még az S-pont tartományon (S0-S8) belül vagyunk.
    while (met > 0 && tik < MeterBarSPointLimit) {
        if (tik == 0) {                                            // Első sáv: S0 (piros)
            int draw_width = std::min(met, (int)MeterBarRedWidth); // Max. a sáv szélessége, vagy amennyi 'met' van
            if (draw_width > 0) {
                tft.fillRect(smeterX + MeterBarRedStartX, smeterY + MeterBarY, draw_width, MeterBarHeight, TFT_RED);
                end_of_colored_x_abs = smeterX + MeterBarRedStartX + draw_width; // Frissítjük a színes rész végét
            }
            met -= MeterBarRedWidth; // Teljes S0 sáv "költségét" levonjuk a 'met'-ből
        } else {                     // Következő sávok: S1-S8 (narancs)
            // X pozíció: MeterBarOrangeStartX + (aktuális narancs sáv indexe) * (narancs sáv szélessége + rés)
            int current_bar_x = smeterX + MeterBarOrangeStartX + ((tik - 1) * MeterBarOrangeSpacing);
            int draw_width = std::min(met, (int)MeterBarOrangeWidth);
            if (draw_width > 0) {
                tft.fillRect(current_bar_x, smeterY + MeterBarY, draw_width, MeterBarHeight, TFT_ORANGE);
                end_of_colored_x_abs = current_bar_x + draw_width;
            }
            met -= MeterBarOrangeWidth; // Teljes narancs sáv "költségét" levonjuk
        }
        tik++; // Lépünk a következő sávra
    }

    // Zöld (S9+10dB - S9+60dB) sávok rajzolása
    // Ciklus amíg van 'met' ÉS még az S9+dB tartományon belül vagyunk.
    while (met > 0 && tik < MeterBarTotalLimit) {
        // X pozíció: MeterBarGreenStartX + (aktuális zöld sáv indexe az S9+dB tartományon belül) * (zöld sáv szélessége + rés)
        int current_bar_x = smeterX + MeterBarGreenStartX + ((tik - MeterBarSPointLimit) * MeterBarGreenSpacing);
        int draw_width = std::min(met, (int)MeterBarGreenWidth);
        if (draw_width > 0) {
            tft.fillRect(current_bar_x, smeterY + MeterBarY, draw_width, MeterBarHeight, TFT_GREEN);
            end_of_colored_x_abs = current_bar_x + draw_width;
        }
        met -= MeterBarGreenWidth; // Teljes zöld sáv "költségét" levonjuk
        tik++;                     // Lépünk a következő sávra
    }

    // Utolsó, S9+60dB feletti narancs sáv rajzolása
    // Ha elértük az összes S és S9+dB sáv végét (tik == MeterBarTotalLimit) ÉS még mindig van 'met' (energia).
    if (tik == MeterBarTotalLimit && met > 0) {
        int draw_width = std::min(met, (int)MeterBarFinalOrangeWidth);
        if (draw_width > 0) {
            tft.fillRect(smeterX + MeterBarFinalOrangeStartX, smeterY + MeterBarY, draw_width, MeterBarHeight, TFT_ORANGE);
            end_of_colored_x_abs = smeterX + MeterBarFinalOrangeStartX + draw_width;
        }
        // met -= MeterBarFinalOrangeWidth; // Itt már nem kell csökkenteni, mert ez az utolsó lehetséges színes sáv.
    }

    // A mérősáv teljes definiált végének X koordinátája (ahol a fekete kitöltésnek véget kell érnie).
    int meter_display_area_end_x_abs = smeterX + MeterBarRedStartX + MeterBarMaxPixelValue;

    // Biztosítjuk, hogy a kirajzolt színes rész ne lógjon túl a definiált maximális értéken.
    if (end_of_colored_x_abs > meter_display_area_end_x_abs) {
        end_of_colored_x_abs = meter_display_area_end_x_abs;
    }
    // Ha spoint=0 volt, akkor semmi sem rajzolódott, end_of_colored_x_abs a skála elején maradt.
    if (spoint == 0) {
        end_of_colored_x_abs = smeterX + MeterBarRedStartX;
    }

    // Fekete kitöltés: az utolsó színes sáv végétől a skála definiált végéig.
    // Csak akkor rajzolunk feketét, ha a színes sáv nem érte el a skála végét.
    if (end_of_colored_x_abs < meter_display_area_end_x_abs) {
        tft.fillRect(end_of_colored_x_abs, smeterY + MeterBarY, meter_display_area_end_x_abs - end_of_colored_x_abs, MeterBarHeight, TFT_BLACK);
    }
}

/**
 * S-Meter skála kirajzolása (a statikus részek: vonalak, számok).
 * Ezt általában egyszer kell meghívni a képernyő inicializálásakor.
 */
void SMeter::drawSmeterScale() {
    using namespace SMeterConstants;

    // Ha már inicializáltuk a pozíciókat, ne rajzoljuk újra (opcionális optimalizáció)
    static bool scale_initialized = false;
    if (scale_initialized && rssi_value_x_pos != 0) {
        return; // Már inicializált, nem kell újra kirajzolni
    }

    tft.setFreeFont();  // Alapértelmezett font használata
    tft.setTextSize(1); // A skála teljes területének törlése feketével (beleértve a szöveg helyét is)
    tft.fillRect(smeterX + ScaleStartXOffset, smeterY + ScaleStartYOffset, ScaleWidth, ScaleHeight + 10, TFT_BLACK);
    tft.setFreeFont(); // Alapértelmezett font használata
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Szövegszín: fehér, Háttér: fekete

    // S-pont skála vonalak és számok (0-9)
    for (int i = 0; i < SPointCount; i++) {
        tft.fillRect(smeterX + SPointStartX + (i * SPointSpacing), smeterY + SPointY, SPointTickWidth, SPointTickHeight, TFT_WHITE);
        // setCursor + print használata a konzisztencia érdekében
        int textX = smeterX + SPointStartX + (i * SPointSpacing) - 3; // Központosítás
        int textY = smeterY + SPointNumberY;
        tft.setCursor(textX, textY);
        tft.print(i);
    }
    // S9+dB skála vonalak és számok (+10, +20, ..., +60)
    for (int i = 1; i <= PlusScaleCount; i++) {
        tft.fillRect(smeterX + PlusScaleStartX + (i * PlusScaleSpacing), smeterY + PlusScaleY, PlusScaleTickWidth, PlusScaleTickHeight, TFT_RED);
        if (i % 2 == 0) {                                                       // Csak minden másodiknál írjuk ki a "+számot" (pl. +20, +40, +60)
            int textX = smeterX + PlusScaleStartX + (i * PlusScaleSpacing) - 8; // Központosítás
            int textY = smeterY + PlusScaleNumberY;
            tft.setCursor(textX, textY);
            tft.print("+");
            tft.print(i * 10);
        }
    }

    // Skála alatti vízszintes sávok
    tft.fillRect(smeterX + SPointStartX, smeterY + SBarY, SBarSPointWidth, SBarHeight, TFT_WHITE); // S0-S9 sáv
    tft.fillRect(smeterX + SBarPlusStartX, smeterY + SBarY, SBarPlusWidth, SBarHeight, TFT_RED);   // S9+dB sáv

    // Statikus RSSI és SNR feliratok kirajzolása - komplett TFT állapot újrabeállítása
    text_y_pos = smeterY + ScaleEndYOffset + 2;
    uint16_t current_x_calc = smeterX + RssiLabelXOffset; // Teljes TFT állapot tisztítása és újrabeállítása a címkékhez
    tft.setFreeFont();                                    // Alapértelmezett font
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN, TFT_COLOR_BACKGROUND); // Feliratok színe
    text_h = tft.fontHeight();                         // Szöveg magassága a törléshez

    // RSSI Felirat - setCursor + print használata
    const char *rssi_label_text = "RSSI: ";
    tft.setCursor(current_x_calc, text_y_pos);
    tft.print(rssi_label_text);
    rssi_label_x_pos = current_x_calc;
    rssi_value_x_pos = current_x_calc + tft.textWidth(rssi_label_text);
    rssi_value_max_w = tft.textWidth("XXX dBuV");              // Max lehetséges szélesség
    current_x_calc = rssi_value_x_pos + rssi_value_max_w + 10; // 10px rés    // SNR Felirat - setCursor + print használata
    const char *snr_label_text = "SNR: ";
    tft.setCursor(current_x_calc, text_y_pos);
    tft.print(snr_label_text);
    snr_label_x_pos = current_x_calc;
    snr_value_x_pos = current_x_calc + tft.textWidth(snr_label_text);
    snr_value_max_w = tft.textWidth("XXX dB"); // Max lehetséges szélesség

    // Jelöljük, hogy a skála már inicializálva van
    scale_initialized = true;
}

/**
 * S-Meter érték és RSSI/SNR szöveg megjelenítése.
 * @param rssi Aktuális RSSI érték (0–127 dBμV).
 * @param snr Aktuális SNR érték (0–127 dB).
 * @param isFMMode Igaz, ha FM módban vagyunk, hamis egyébként (AM/SSB/CW).
 */
void SMeter::showRSSI(uint8_t rssi, uint8_t snr, bool isFMMode) {

    // 1. Dinamikus S-Meter sávok kirajzolása az aktuális RSSI alapján
    drawMeterBars(rssi, isFMMode); // Ez már tartalmazza a prev_spoint_bars optimalizációt

    // 2. Ellenőrizzük, hogy a pozíciók inicializálva vannak-e
    if (rssi_value_x_pos == 0 && snr_value_x_pos == 0) {
        // Ha a pozíciók még nincsenek beállítva, inicializáljuk a skálát
        drawSmeterScale();
    }

    // 3. RSSI és SNR értékek szöveges kiírása, csak ha változott az értékük
    bool rssi_changed = (rssi != prev_rssi_for_text);
    bool snr_changed = (snr != prev_snr_for_text);

    if (!rssi_changed && !snr_changed)
        return; // Ha semmi sem változott, kilépünk// Font és egyéb beállítások az értékekhez - teljes TFT állapot újrabeállítása
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND); // Értékek színe: fehér, háttér: fekete (felülíráshoz)

    if (rssi_changed) {
        char rssi_str_buff[12]; // "XXX dBuV" + null
        snprintf(rssi_str_buff, sizeof(rssi_str_buff), "%3d dBuV", rssi);
        // Régi érték területének törlése
        tft.fillRect(rssi_value_x_pos, text_y_pos, rssi_value_max_w, text_h, TFT_COLOR_BACKGROUND);
        // Új érték kirajzolása - setCursor + print használata
        tft.setCursor(rssi_value_x_pos, text_y_pos);
        tft.print(rssi_str_buff);
    }

    if (snr_changed) {
        char snr_str_buff[10]; // "XXX dB" + null
        snprintf(snr_str_buff, sizeof(snr_str_buff), "%3d dB", snr);
        // Régi érték területének törlése
        tft.fillRect(snr_value_x_pos, text_y_pos, snr_value_max_w, text_h, TFT_COLOR_BACKGROUND);
        // Új érték kirajzolása - setCursor + print használata
        tft.setCursor(snr_value_x_pos, text_y_pos);
        tft.print(snr_str_buff);
    }

    // Elmentjük az aktuális numerikus értékeket a következő összehasonlításhoz
    if (rssi_changed) {
        prev_rssi_for_text = rssi;
    }

    if (snr_changed) {
        prev_snr_for_text = snr;
    }
}
