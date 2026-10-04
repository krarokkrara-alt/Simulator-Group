import fs from 'node:fs';
import vm from 'node:vm';

const source=fs.readFileSync(new URL('../ESP32_WiFi_StandAlone_Test/WebPage.h',import.meta.url),'utf8');
const script=source.match(/<script>([\s\S]*?)<\/script>/)[1];
export const flush=()=>new Promise(resolve=>setImmediate(resolve));
const deferred=()=>{let resolve,reject;const promise=new Promise((r,e)=>{resolve=r;reject=e});return {promise,resolve,reject};};
export const response=(data,status=200)=>({ok:status<400,status,json:async()=>structuredClone(data)});
export async function setup(){
 const elements=new Map(),listeners=new Map(),commands=[],polls=[];
 let authoritative={version:'test',running:false,rpm:1000,profile:0,ckp:true,cmp:true,led:false,scope:false,tps:10,map:40,ect:50,iat:40,o2:50};
 let holdPolls=false;
 const document={activeElement:null,addEventListener(type,fn){if(!listeners.has(type))listeners.set(type,[]);listeners.get(type).push(fn)},getElementById(id){
  if(!elements.has(id))elements.set(id,{id,type:['rpm','tps','map','ect','iat','o2'].includes(id)?'range':'',value:0,textContent:'',innerHTML:'',style:{},classList:{toggle(){}}});return elements.get(id);
 }};
 const context=vm.createContext({document,setInterval(){},fetch(url,options={}){
  if(url==='/api/profiles')return Promise.resolve(response([]));
  if(url==='/api/set'){const item=deferred();commands.push({...item,body:options.body});return item.promise;}
  if(url==='/api/state'){const snapshot=structuredClone(authoritative);if(!holdPolls)return Promise.resolve(response(snapshot));const item=deferred();polls.push({...item,snapshot});return item.promise;}
  throw Error('Unexpected endpoint');
 }});
 vm.runInContext(script,context);await flush();
 return {context,document,commands,polls,get:document.getElementById,
  emit(type,target,extra={}){for(const fn of listeners.get(type)||[])fn({target,...extra})},
  edit(id,value){const control=document.getElementById(id);control.value=value;for(const fn of listeners.get('input')||[])fn({target:control});context.show(control);return control;},
  inspect(expression){return vm.runInContext(expression,context);},
  holdPolls(value){holdPolls=value;},
  reply(index,patch={},status=200,apply=true){const data=status===200?{...authoritative,...patch}:patch;if(status===200&&apply)authoritative=structuredClone(data);commands[index].resolve(response(data,status));},
 };
}
