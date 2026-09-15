#include "audio_manager.h"
#include "pins.h"

static AudioManager* s_instance = nullptr;

void audio_eof_speech(const char*) {
  if (s_instance) s_instance->stopPlaying();
}

void audio_eof_mp3(const char*) {
  if (s_instance) s_instance->stopPlaying();
}

void audio_eof_stream(const char*) {
  if (s_instance) s_instance->stopPlaying();
}

void AudioManager::initSpeaker() {
  s_instance = this;
  audio_.setPinout(AMP_BCLK, AMP_LRC, AMP_DOUT);
  audio_.setVolume(24);
  Serial.println("[audio] speaker ok");
}

void AudioManager::initMic() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = true
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = MIC_SCK,
    .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD
  };

  i2s_driver_install(I2S_MIC, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_MIC, &pin_config);
  i2s_set_clk(I2S_MIC, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void AudioManager::tick() {
  audio_.loop();

  if (!playing_ && !queue_.empty()) {
    playing_ = true;
    play_start_ = millis();
    String payload = queue_.dequeue();

    audio_.connecttospeech(payload.c_str(), tts_lang_.c_str());
  }

  if (playing_ && (millis() - play_start_ > PLAY_TIMEOUT)) {
    Serial.println("[audio] playback timeout");
    audio_.stopSong();
    playing_ = false;
  }
}

bool AudioManager::isPlaying() const {
  return playing_;
}

void AudioManager::stopPlaying() {
  playing_ = false;
}

void AudioManager::setTtsLang(const String& lang) {
  tts_lang_ = lang;
}

int AudioManager::readMic(int16_t* samples, size_t max_samples) {
  size_t bytes_read = 0;
  i2s_read(I2S_MIC, samples, max_samples * sizeof(int16_t), &bytes_read, pdMS_TO_TICKS(10));
  return (int)bytes_read;
}
