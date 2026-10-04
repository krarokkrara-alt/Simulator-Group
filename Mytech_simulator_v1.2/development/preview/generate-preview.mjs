import fs from 'node:fs';
import crypto from 'node:crypto';

const sourceDir = new URL('../ESP32_WiFi_StandAlone_Test/', import.meta.url);
const page = fs.readFileSync(new URL('WebPage.h', sourceDir), 'utf8');
const match = page.match(/R"HTML\(([\s\S]*?)\)HTML";/);
if (!match) throw new Error('Cannot extract INDEX_HTML raw string from WebPage.h.');
const html = match[1].trim();
if (!html.includes('<body>') || !html.includes('<script>')) throw new Error('Unexpected source HTML structure.');
const profileSource = fs.readFileSync(new URL('VehicleProfiles.cpp', sourceDir), 'utf8');
const table = profileSource.match(/PROFILES\[\]\s*=\s*\{([\s\S]*?)\n\};/)[1];
const quoted = '"(?:[^"\\\\]|\\\\.)*"';
const profilePattern = new RegExp('\\{\\s*('+quoted+'),\\s*('+quoted+'),\\s*('+quoted+'),\\s*TriggerSensorType::(Hall|MagneticVR),\\s*TriggerSensorType::(Hall|MagneticVR),\\s*('+quoted+'),\\s*('+quoted+')\\s*\\}', 'g');
const profiles = [...table.matchAll(profilePattern)].map((p,id)=>({
  id, brand:JSON.parse(p[1]), model:JSON.parse(p[2]), engine:JSON.parse(p[3]),
  ckpType:p[4]==='Hall'?'Hall effect':'Magnetic VR', cmpType:p[5]==='Hall'?'Hall effect':'Magnetic VR',
  ckpPattern:JSON.parse(p[6]), notes:JSON.parse(p[7])
}));
if (!profiles.length || profiles.length !== (table.match(/TriggerSensorType::/g)||[]).length/2) throw new Error('Profile extraction incomplete; update generator before regenerating.');
const configSource = fs.readFileSync(new URL('AppConfig.h', sourceDir), 'utf8');
const config = {
  version:configSource.match(/FW_VERSION\[\]\s*=\s*"([^"]+)"/)[1],
  maxRpm:Number(configSource.match(/MAX_RPM\s*=\s*(\d+)/)[1]),
  profiles,
  sourceSha256:crypto.createHash('sha256').update(page).digest('hex')
};
const mock = fs.readFileSync(new URL('offline-mock.js', import.meta.url), 'utf8');
const configJson = JSON.stringify(config).replaceAll('<','\\u003c');
const banner = '<aside id="offlinePreviewBanner" style="position:sticky;top:0;z-index:1000;padding:18px;margin-bottom:18px;border:3px solid #fbbf24;border-radius:12px;background:#422006;color:#fef3c7;line-height:1.6"><strong style="font-size:1.2rem">ตัวอย่างจำลอง — ไม่เชื่อมต่อบอร์ด/ไม่ใช่ผลทดสอบฮาร์ดแวร์</strong><br>ตรวจหน้าจอและลองปุ่มได้ออฟไลน์ ค่าและสถานะทั้งหมดจำลองในเบราว์เซอร์ ไม่สร้างสัญญาณ GPIO ไม่เชื่อมต่อ Wi-Fi และไม่แฟลชบอร์ด</aside>';
const output = html.replace('<head>', '<head><meta charset="utf-8">')
  .replace('<body>', '<body>'+banner)
  .replace('<script>', '<script>window.__offlinePreviewConfig='+configJson+';\n'+mock+'\n</script>\n<script>');
fs.writeFileSync(new URL('index.html', import.meta.url), output+'\n', 'utf8');
console.log(`Offline preview generated from source UI (${config.sourceSha256}); ${profiles.length} profiles. No network or board access.`);
