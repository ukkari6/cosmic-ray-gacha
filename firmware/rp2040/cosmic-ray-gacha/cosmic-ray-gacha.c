/*
宇宙線ガチャ ver3.00000000000（バージョンサン那由他（なゆた）

CMakeLists.txtで
最適化レベル0（-O0）Debugビルド
に設定してデータを省略させないようにしています。

*/
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

// --- UARTの設定 ---
#define UART_ID       uart1
#define BAUD_RATE     115200
#define UART_TX_PIN   4
#define UART_RX_PIN   5

// --- UART送信スイッチ ---
#define TRIGGER_BUTTON_PIN 15 

//デフォルトLED
const uint LED_PIN = PICO_DEFAULT_LED_PIN;

// --- 宇宙線監視用の配列
//最大RAM264kb
#define GACHA_POOL_SIZE (250 * 1024)    //250kb
//volatileで最適化防止
volatile uint8_t gacha_memory_pool[GACHA_POOL_SIZE];


// 宇宙線監視用配列を送信
bool send_gacha_memory(void){
    // UART処理
    gpio_put(LED_PIN, 1);

    // Python側ヘッダー送信
    uart_putc(UART_ID, 0xC0);
    uart_putc(UART_ID, 0x51);
    uart_putc(UART_ID, 0x1C);
    uart_putc(UART_ID, 0x1A);

    // データサイズ（4バイト）ビッグエンディアンで送信
    uart_putc(UART_ID, (GACHA_POOL_SIZE >> 24) & 0xFF);
    uart_putc(UART_ID, (GACHA_POOL_SIZE >> 16) & 0xFF);
    uart_putc(UART_ID, (GACHA_POOL_SIZE >> 8)  & 0xFF);
    uart_putc(UART_ID, (GACHA_POOL_SIZE)       & 0xFF);

    // PythonからのACK (0x06) 受信待ち
    // タイムアウト監視用（ミリ秒単位）
    uint32_t timeout_ms = 5000; // 5秒応答がなければ諦める
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    bool ack_received = false;

    while (to_ms_since_boot(get_absolute_time()) - start_time < timeout_ms) {
        watchdog_update(); // 待機中もウォッチドッグをクリア

        if (uart_is_readable(UART_ID)) {
            uint8_t rx_byte = uart_getc(UART_ID);
            if (rx_byte == 0x06) { // ACKコード 0x06
                ack_received = true;
                break;
            }
        }
        tight_loop_contents(); // 軽いウェイト
    }

    // ACKが受信できなかった場合はデータ送信を中止して戻る
    if (!ack_received) {
        gpio_put(LED_PIN, 0);
        return false; 
    }

    // ACKが確認できたら送信を開始
    // 1バイトずつUARTから送り出す
    // 確実に送れるようにbusy_wait_usを入れる
    for (int i = 0; i < GACHA_POOL_SIZE; i++) {
        uart_putc(UART_ID, gacha_memory_pool[i]);
        busy_wait_us(100);
        watchdog_update();
    }

    //送信完了したらLEDチカチカさせる
    for(int i = 0; i <= 5; i++){
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    return true;
}

int main()
{
    stdio_init_all();

    // UART1の初期化とピン設定
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // 物理スイッチのピン設定（内部プルアップ）
    gpio_init(TRIGGER_BUTTON_PIN);
    gpio_set_dir(TRIGGER_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(TRIGGER_BUTTON_PIN);

    // LEDのピン設定
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //宇宙線監視用の配列を0x55と0xAAの交互に初期化
    for (int i = 0; i < GACHA_POOL_SIZE; i++) {
        if (i % 2 == 0) {
            // iが偶数
            gacha_memory_pool[i] = 0x55;
        }else{
            //iが奇数
            gacha_memory_pool[i] = 0xAA;
        }
}

    //2秒間LEDを点灯させる
    gpio_put(LED_PIN,1);
    sleep_ms(2000);

    // ウォッチドッグタイマを有効化（8秒）
    watchdog_enable(8000, 1);

    // Lチカ（点滅）制御用の変数
    uint32_t last_blink_time = 0;
    bool led_state = false;

    // メインループ
    while (true) {
        watchdog_update();

        // 1秒（1000ms）周期で制御
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        uint32_t elapsed = current_time - last_blink_time;

        if (elapsed >= 1000) {
            // 1秒経ったら次の周期のためにタイマーを更新
            last_blink_time = current_time;
            elapsed = 0;
        }

        // 周期の始まりから10ms以内だけON、それ以外（990ms）はOFFにする（省エネ目的）
        if (elapsed < 10) {
            gpio_put(LED_PIN, true);
        } else {
            gpio_put(LED_PIN, false);
        }

        // スイッチが押されたら宇宙線監視用配列を送信
        if (gpio_get(TRIGGER_BUTTON_PIN) == 0) {
            sleep_ms(1000); // 1秒以上長押しで送信開始
            if (gpio_get(TRIGGER_BUTTON_PIN) == 0) {
                send_gacha_memory();
            }
        }
        
        sleep_ms(1);
    }
}