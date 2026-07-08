#!/usr/bin/env python3
import serial
import argparse
import sys

def check_cosmic_ray_gacha(port, baudrate):
    print(f"[*] シリアルポート {port} ({baudrate}bps) を開いています...")
    try:
        # FT232RLのバッファを詰まらせないよう、一括受信時のタイムアウトを長めの10秒に設定
        ser = serial.Serial(port, baudrate, timeout=10.0)
    except Exception as e:
        print(f"[!] ポートを開けませんでした: {e}")
        return
    
    # --- 1. 開始信号（マジックナンバー: 0xC0, 0x51, 0x1C, 0x1A）の同期 ---
    MAGIC_HEADER = [0xC0, 0x51, 0x1C, 0x1A]
    state = 0
    
    print("[*] マイコンからのヘッダー (0xC0511C1A) を待機中...")
    
    while True:
        b = ser.read(1)
        if not b:
            continue
        byte_val = b[0]
        
        if byte_val == MAGIC_HEADER[state]:
            state += 1
            if state == len(MAGIC_HEADER):
                break
        else:
            if byte_val == MAGIC_HEADER[0]:
                state = 1
            else:
                state = 0

    # --- 2. データサイズ（4バイト）を受信して解析 ---
    size_bytes = ser.read(4)
    if len(size_bytes) < 4:
        print("[!] エラー: サイズ情報の受信に失敗しました。")
        ser.close()
        return
    
    gacha_pool_size = int.from_bytes(size_bytes, byteorder='big')
    print(f"[+] 転送サイズ: {gacha_pool_size} バイト")
    print(f"[*] 総監視ビット数: {gacha_pool_size * 8:,} ビットの検証を開始します...")

    # --- 3. 申告されたサイズ分を小分け（チャンク）に受信 ---
    print(f"[*] メモリデータの受信を開始します...")
    received_bytes = bytearray()
    chunk_size = 256
    
    while len(received_bytes) < gacha_pool_size:
        remaining = gacha_pool_size - len(received_bytes)
        to_read = min(chunk_size, remaining)
        
        chunk = ser.read(to_read)
        if not chunk:
            print(f"\n[!] エラー: データが途中で途切れました。({len(received_bytes)} / {gacha_pool_size} バイト受信)")
            ser.close()
            return
            
        received_bytes.extend(chunk)
        
        progress = (len(received_bytes) / gacha_pool_size) * 100
        sys.stdout.write(f"\r  受信中: {len(received_bytes)} / {gacha_pool_size} バイト ({progress:.1f}%)")
        sys.stdout.flush()

    ser.close()
    print(f"\n[+] データ受信完了。検証中...")

    # --- 4. メモリチェック（市松模様対応・全ビットスキャン） ---
    error_count = 0
    flipped_bits_details = []

    for address, byte_value in enumerate(received_bytes):
        # マイコン側の if (i % 2 == 0) と同じルールで正解を逆算
        expected_value = 0x55 if (address % 2 == 0) else 0xAA
        
        # 初期値と食い違っている場合（宇宙線ヒット）
        if byte_value != expected_value:
            error_count += 1
            # XOR演算で「変化（反転）があったビット」だけを 1 にしたマスクを作る
            diff_mask = byte_value ^ expected_value
            
            for bit in range(8):
                # 化けたビットの場所を特定
                if (diff_mask >> bit) & 1:
                    # 宇宙線衝突後に「1になった」か「0になった」かも判別
                    final_bit_state = (byte_value >> bit) & 1
                    flipped_bits_details.append((address, bit, expected_value, final_bit_state))

    # --- 5. 結果報告 ---
    print("\n" + "="*50)
    print("宇宙線ガチャ 検証結果")
    print("="*50)
    if error_count == 0:
        print(" 【結果】エラーは検出されませんでした。")
    else:
        print(f"エラー検出")
        print(f" 異常バイト数: {error_count} バイト")
        print("\n--- 反転セルの物理アドレス詳細 ---")
        for addr, bit, exp, current in flipped_bits_details:
            # 該当アドレスの実際の受信バイトを取得
            actual_byte = received_bytes[addr]
            
            # 元のビット状態を割り出す
            original_bit = (exp >> bit) & 1
            
            print(f" ・アドレス [0x{addr:05X}] の {bit}番目 ビットが変異: {original_bit} ➔ {current}")
            
            # 【追加】16進数と2進数でのデータ比較表示
            print(f"   [16進数] 期待値: 0x{exp:02X} ➔ 受信値: 0x{actual_byte:02X}")
            print(f"   [2進数]  期待値: {exp:08b}b")
            print(f"            受信値: {actual_byte:08b}b")
            print("-" * 40)
    print("="*50)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="宇宙線ガチャ")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="シリアルポートのパス (デフォルト: /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="ボーレート (デフォルト: 115200)")
    args = parser.parse_args()
    
    check_cosmic_ray_gacha(args.port, args.baud)