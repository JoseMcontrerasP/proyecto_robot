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
    /*modulo 1*/
    document.getElementById("valor 110").innerHTML= data["sensor_1"];
    /*los siguientes*/
    if(!document.getElementById("Modulo 2")){
        console.log("no se encuentra el modulo 2");
    }
    else{
        const data2  = await getjson('http://192.168.1.101/sensores.json');
        document.getElementById("valor 210").innerHTML= data2["sensor_1"];
    }


}

let ides =[];
let modulos ={}
document.getElementById("agregarmodulo").addEventListener("click", creartabla);
var t=setInterval(recibir,1000);