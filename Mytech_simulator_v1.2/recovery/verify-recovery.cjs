// Checks saved files against text reconstructed from the visible shared-chat diffs.
// FNV is a transfer check, not proof of original file/commit byte identity.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const manifest = JSON.parse(fs.readFileSync(path.join(__dirname,'source-manifest.json'),'utf8'));
const root = path.resolve(__dirname,'../ESP32_WiFi_StandAlone_Test');
const result = manifest.sourceManifest.map(expected => {
  const file = path.join(root,expected.name);
  if (!fs.existsSync(file)) return {name:expected.name,pass:false,reason:'missing'};
  const bytes = fs.readFileSync(file);
  const text = bytes.toString('utf8').replace(/\r\n/g,'\n');
  let h=2166136261;
  for(let i=0;i<text.length;i++) h=Math.imul(h^text.charCodeAt(i),16777619)>>>0;
  const hash=h.toString(16).padStart(8,'0');
  const lines=text.split('\n').length-(text.endsWith('\n')?1:0);
  return {name:expected.name,pass:hash===expected.fnv1aUtf16&&text.length===expected.utf16Length&&lines===expected.lines,lines,utf16Length:text.length,fnv1aUtf16:hash,sha256:crypto.createHash('sha256').update(bytes).digest('hex')};
});
console.log(JSON.stringify({checkedAt:new Date().toISOString(),files:result,passed:result.filter(x=>x.pass).length,total:result.length},null,2));
process.exitCode=result.every(x=>x.pass)?0:1;
