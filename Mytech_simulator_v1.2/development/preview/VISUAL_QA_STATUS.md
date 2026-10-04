# Offline preview visual QA — 4 ตุลาคม 2026

สถานะ: **browser visual QA ยังทำไม่ได้ใน environment นี้**
อ่าน computer-use SKILL.md และ cua browser documentation ก่อนเรียก browser
พยายามเปิด local preview file ผ่าน cua.createBrowserTab('iab',file URL,visible:false) ได้ `Browser is not available: iab`
ตรวจ cua.getState ต่อหนึ่งครั้งได้ apps=[] และ browsers=[] จึงหยุด ไม่ติดตั้ง browser/runtime และไม่เปิด localhost server ที่ไม่มี browser ใช้

ยังไม่ได้ render/เห็น layout จริง ไม่มี screenshot artifact และไม่ได้คลิก START/STOP, RPM หรือ sensors ใน browser
ไม่มีหลักฐาน native touch/pointer interaction, viewport responsiveness หรือปุ่มถูก bannerบัง/ไม่บัง
ไม่แก้ preview หรือ firmware และไม่เชื่อม external network/บอร์ด

เมื่อ browser พร้อม ให้เปิด index.html ออฟไลน์ ตรวจ banner MOCK ชัดเจน, status/controls และ viewportเล็ก แล้วทดลอง START/STOP/RPM/sensor/profile/SELFTEST ผ่านmockพร้อม screenshot
ต้องระบุผลmockแยกจาก firmware HTTP transport และ hardware ไม่อ้างว่าคลิกmockพิสูจน์ timing/voltage
source-only/Node mock tests ที่ฝ่ายอื่นทำ ไม่แทน browser visual QA นี้
