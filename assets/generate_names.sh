#!/usr/bin/env bash
set -euo pipefail

MAIN_VOICE="ar-SA-HamedNeural"
EXC_VOICE="ar-SA-ZariyahNeural"
OUTDIR="asmaul_husna"

mkdir -p "$OUTDIR"
cd "$OUTDIR"

trim_audio() {
  local input="$1"
  local output="$2"

  ffmpeg -y -i "$input" \
    -af "silenceremove=start_periods=1:start_duration=0.02:start_threshold=-45dB:start_silence=0:stop_periods=1:stop_duration=0.05:stop_threshold=-45dB:stop_silence=0,asetpts=PTS-STARTPTS" \
    -c:a libmp3lame -q:a 2 "$output" >/dev/null 2>&1
}

make_normal() {
  local filename="$1"
  local text="$2"
  local tmp="_${filename}.mp3"

  echo "Generating ${filename}.mp3"
  edge-tts \
    --voice "$MAIN_VOICE" \
    --rate=-20% \
    --text "$text" \
    --write-media "$tmp"

  trim_audio "$tmp" "${filename}.mp3"
  rm -f "$tmp"
}

make_special() {
  local filename="$1"
  local phrase="$2"
  local tmp="_${filename}.mp3"

  echo "Generating ${filename}.mp3 (special)"
  edge-tts \
    --voice "$EXC_VOICE" \
    --rate=-30% \
    --pitch=-10Hz \
    --text "$phrase" \
    --write-media "$tmp"

  trim_audio "$tmp" "${filename}.mp3"
  rm -f "$tmp"
}

while IFS='|' read -r filename text; do
  [ -z "$filename" ] && continue

  case "$filename" in
    Allah)
      make_special "$filename" "اللَّهُ رَبِّي"
      ;;
    Ad-Darr)
      make_special "$filename" "الضَّارُّ النَّافِعُ"
      ;;
    *)
      make_normal "$filename" "$text"
      ;;
  esac
done <<'EOF'
Allah|اللَّه
Ar-Rahman|الرَّحْمَان
Ar-Raheem|الرَّحِيم
Al-Malik|المَلِك
Al-Quddus|القُدُّوس
As-Salam|السَّلَام
Al-Mumin|المُؤْمِن
Al-Muhaymin|المُهَيْمِن
Al-Aziz|العَزِيز
Al-Jabbar|الجَبَّار
Al-Mutakabbir|المُتَكَبِّر
Al-Khaliq|الخَالِق
Al-Bari|البَارِئ
Al-Musawwir|المُصَوِّر
Al-Ghaffar|الغَفَّار
Al-Qahhar|القَهَّار
Al-Wahhab|الوَهَّاب
Ar-Razzaq|الرَّزَّاق
Al-Fattah|الفَتَّاح
Al-Alim|العَلِيم
Al-Qabid|القَابِض
Al-Basit|البَاسِط
Al-Khafid|الخَافِض
Ar-Rafi|الرَّافِع
Al-Muizz|المُعِزّ
Al-Mudhill|المُذِلّ
As-Sami|السَّمِيع
Al-Basir|البَصِير
Al-Hakam|الحَكَم
Al-Adl|العَدْل
Al-Latif|اللَّطِيف
Al-Khabir|الخَبِير
Al-Halim|الحَلِيم
Al-Azim|العَظِيم
Al-Ghafur|الغَفُور
Ash-Shakur|الشَّكُور
Al-Ali|العَلِيّ
Al-Kabir|الكَبِير
Al-Hafiz|الحَفِيظ
Al-Muqit|المُقِيت
Al-Hasib|الحَسِيب
Al-Jalil|الجَلِيل
Al-Karim|الكَرِيم
Ar-Raqib|الرَّقِيب
Al-Mujib|المُجِيب
Al-Wasi|الوَاسِع
Al-Hakim|الحَكِيم
Al-Wadud|الوَدُود
Al-Majid|المَجِيد
Al-Baith|البَاعِث
Ash-Shahid|الشَّهِيد
Al-Haqq|الحَقّ
Al-Wakil|الوَكِيل
Al-Qawi|القَوِيّ
Al-Matin|المَتِين
Al-Wali|الوَلِيّ
Al-Hamid|الحَمِيد
Al-Muhsi|المُحْصِي
Al-Mubdi|المُبْدِئ
Al-Muid|المُعِيد
Al-Muhyi|المُحْيِي
Al-Mumit|المُمِيت
Al-Hayy|الحَيّ
Al-Qayyum|القَيُّوم
Al-Wajid|الوَاجِد
Al-Majid2|المَاجِد
Al-Wahid|الوَاحِد
Al-Ahad|الأَحَد
As-Samad|الصَّمَد
Al-Qadir|القَادِر
Al-Muqtadir|المُقْتَدِر
Al-Muqaddim|المُقَدِّم
Al-Muakhkhir|المُؤَخِّر
Al-Awwal|الأَوَّل
Al-Akhir|الآخِر
Az-Zahir|الظَّاهِر
Al-Batin|البَاطِن
Al-Wali2|الوَالِي
Al-Mutaali|المُتَعَالِي
Al-Barr|البَرّ
At-Tawwab|التَّوَّاب
Al-Muntaqim|المُنْتَقِم
Al-Afuw|العَفُوّ
Ar-Rauf|الرَّؤُوف
Malik-ul-Mulk|مَالِكُ المُلْك
Dhul-Jalali-wal-Ikram|ذُو الجَلَالِ وَالإِكْرَام
Al-Muqsit|المُقْسِط
Al-Jami|الجَامِع
Al-Ghani|الغَنِيّ
Al-Mughni|المُغْنِي
Al-Mani|المَانِع
Ad-Darr|الضَّارُّ
An-Nafi|النَّافِع
An-Nur|النُّور
Al-Hadi|الهَادِي
Al-Badi|البَدِيع
Al-Baqi|البَاقِي
Al-Warith|الوَارِث
Ar-Rashid|الرَّشِيد
As-Sabur|الصَّبُور
EOF

echo "Done."