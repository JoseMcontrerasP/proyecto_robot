function numids(){
    const idnode    =   document.getElementById("bloquesitos");
    let nextid      =   idnode.lastElementChild.id;
    let numero      =   nextid[7];
    numero++;
    return numero;
}
function creartabla(){
    const node  =   document.getElementById("modulo 1");
    const clone =   node.cloneNode(true);
    let numeroid = numids();
    
    clone.id="Modulo " + numeroid; 
    modulos["idmod"] = clone.id;
    
    document.getElementById("bloquesitos").appendChild(clone);
    document.getElementById(clone.id).children[0].innerHTML=clone.id;

    let ztable = document.getElementById(clone.id).children;
    for(let i = 1;i<(ztable.length);i++){
        let row = ztable[i].children[1].children[1];
        console.log(row.childElementCount);
        for(let r=0;r<row.childElementCount;r++){
            row.children[r].id="valor "+ numeroid +i+r;
            row.children[r].innerHTML="0";
            
        }  
    }
    

}


async function getjson(url){
    const response= await fetch(url);
    return response.json();
}
async function recibir(){

    const data  = await getjson('http://192.168.1.100/sensores.json');
    let algo = Object.keys(data);
    for(let z of algo){
        for(let a = 0; a< data[z].length;a++){
            document.getElementById("valor 1"+z[7]+a).innerHTML= data[z][a];
        }
    }
    /*modulo 1*/
    
    /*los siguientes*/
    if(!document.getElementById("Modulo 2")){
        //console.log("no se encuentra el modulo 2");
    }
    else{
        const data2  = await getjson('http://192.168.1.101/sensores.json');
        algo = Object.keys(data2);
        for(let z of algo){
            for(let a = 0; a< data[z].length;a++){
                document.getElementById("valor 2"+z[7]+a).innerHTML= data2[z][a];
            }
        }
    }


}

let ides =[];
var modulos ={};
document.getElementById("agregarmodulo").addEventListener("click", creartabla);
var t=setInterval(recibir,1000);