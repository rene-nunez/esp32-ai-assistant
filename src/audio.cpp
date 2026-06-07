#include "config.h"
#include "Audio.h"

static Audio audio;
static bool playing = false;
static unsigned long play_start_time = 0;

bool is_playing() {
    return playing;
}

void audio_init() {
    audio.setPinout(AMP_BCLK, AMP_LRC, AMP_DOUT);
    audio.setVolume(24);
    Serial.println("Audio OK");
}

void audio_tick() {
    audio.loop();

    if (!playing && !queue_empty()) {
        playing = true;
        play_start_time = millis();
        String payload = dequeue();

        Serial.print("Playing: ");
        Serial.println(payload);

        if (payload.startsWith("PLAY_TEXT:")) {
            audio.connecttospeech(payload.substring(10).c_str(), "en");
        }
    }

    if (playing && (millis() - play_start_time > PLAY_TIMEOUT)) {
        Serial.println("Playback TIMEOUT — releasing");
        audio.stopSong();
        playing = false;
    }
}

static void on_audio_end() {
    playing = false;
}

void audio_eof_mp3(const char*)    { on_audio_end(); }
void audio_eof_stream(const char*) { on_audio_end(); }
void audio_eof_speech(const char*) { on_audio_end(); }

void i2s_init() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = MIC_SCK,
        .ws_io_num = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_SD
    };

    i2s_driver_install(I2S_MIC, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_MIC, &pin_config);
}

int i2s_read_chunk(int16_t* samples, size_t max_samples) {
    size_t bytes_read = 0;
    i2s_read(I2S_MIC, samples, max_samples * sizeof(int16_t), &bytes_read, pdMS_TO_TICKS(10));
    return (int)bytes_read;
}
