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

// --- 宇宙線監視用の配列
//最大RAM264kb
#define GACHA_POOL_SIZE (250 * 1024)    //250kb
//volatileで最適化防止
volatile uint8_t gacha_memory_pool[GACHA_POOL_SIZE];

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

    // デフォルトLEDのピン設定
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
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

        // スイッチが押されたら送信
        if (gpio_get(TRIGGER_BUTTON_PIN) == 0) {
            sleep_ms(1000); // 1秒以上長押しで送信開始
            if (gpio_get(TRIGGER_BUTTON_PIN) == 0) {
                break;  // while(true)ループから抜けてUART処理へ
            }
        }
        
        sleep_ms(10);
    }


    //UART処理
    gpio_put(LED_PIN,1);

    //Python側ヘッダー送信
    uart_putc(UART_ID, 0xC0);
    uart_putc(UART_ID, 0x51);
    uart_putc(UART_ID, 0x1C);
    uart_putc(UART_ID, 0x1A);

    //データサイズ（4バイト）ビッグエンディアンで送信
    uart_putc(UART_ID, (GACHA_POOL_SIZE >> 24) & 0xFF);
    uart_putc(UART_ID, (GACHA_POOL_SIZE >> 16) & 0xFF);
    uart_putc(UART_ID, (GACHA_POOL_SIZE >> 8)  & 0xFF);
    uart_putc(UART_ID, (GACHA_POOL_SIZE)       & 0xFF);

    //1バイトずつUARTから送り出す
    //確実に送れるようにsleep_msを入れる
    for (int i = 0; i < GACHA_POOL_SIZE; i++) {
        uart_putc(UART_ID, gacha_memory_pool[i]);
        busy_wait_us(100);
        watchdog_update();
    }
    

    // 送信完了後はLEDを点滅させる
    while (true) {
        watchdog_update();
        gpio_put(LED_PIN,1);
        sleep_ms(500);
        gpio_put(LED_PIN,0);
        sleep_ms(500);
    }
}