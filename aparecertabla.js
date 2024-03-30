function numids(){
    const idnode    =   document.getElementById("bloquesitos");
    let nextid      =   idnode.lastElementChild.id;
    let numero      =   nextid[7];
    numero++;
    return numero;
}

function creartabla(){
    const node  =   document.getElementById("Modulo 1");
    const clone =   node.cloneNode(true);
    let numeroid = numids();
    clone.id="Modulo " + numeroid; 

    document.getElementById("bloquesitos").appendChild(clone);
    document.getElementById(clone.id).children[0].innerHTML=clone.id;

    let ztable = document.getElementById(clone.id).children;
    for(let i = 1;i<(ztable.length);i++){
        let row = ztable[i].children[1].children[1];
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
    /*aqui agregar algo que haga ping a los servidores*/
    
    /*todos los modulos*/
    for(let p=1; p<4;p++){ // el 3 es el numero de modulos a evaluer
        if(!document.getElementById("Modulo "+ p)){
        console.log("no se encuentra el modulo "+ p);
        }
        else{
            
            const data  = await getjson('http://192.168.1.10'+p+'/sensores.json');
            let algo = Object.keys(data);
            for(let z of algo){
                for(let a = 0; a < data[z].length;a++){
                    document.getElementById("valor "+p+z[7]+a).innerHTML= data[z][a];
                }
            }
        }
    }
}

let ides =[];
var modulos ={};
document.getElementById("agregarmodulo").addEventListener("click", creartabla);
var t=setInterval(recibir,1000);