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
    document.getElementById(clone.id).children[0].innerHTML="Modulo "+(numeroid-1);

    let medio = document.getElementById(clone.id).children;
    let ztable = medio[1].children;
    console.log(ztable.length);
    for(let i = 1;i<(ztable.length);i++){
        if(ztable[i].nodeName == "TABLE"){
            let row = ztable[i].children[0].children[1];
            for(let r=0;r<row.childElementCount;r++){
                if(i>1){
                    row.children[r].id="valor "+ numeroid +(i-1)+r;
                       
                }else{
                    row.children[r].id="valor "+ numeroid +i+r;   
                }
                row.children[r].innerHTML="0";
            }  
        }
    }  
}

async function getjson(url){
    const response= await fetch(url);
    return response.json();
}

async function recibirsensores(p){
    if(!document.getElementById("Modulo "+ p)){
        //console.log("no se encuentra el modulo "+ p);
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

async function wifi(){
    let url =   "http://192.168.1.101/ping";
    fetch(url)
        .then(async function(response){
            if (response.ok){
                //console.log("ok");
                const potencia  = await getjson('http://192.168.1.101/RSSI.json');
                rssi=potencia["rssi"];
                //console.log(rssi);
                if(rssi<0 && rssi>-30){
                    document.getElementById("wifi").src="img/signal_wifi_4_bar_FILL0_wght400_GRAD0_opsz24.svg";
                }
                else if(rssi<-30 && rssi> -65){
                    document.getElementById("wifi").src="img/network_wifi_3_bar_FILL0_wght400_GRAD0_opsz24.svg";
                }
                else if(rssi <-65 && rssi> -75){
                    document.getElementById("wifi").src="img/network_wifi_2_bar_FILL0_wght400_GRAD0_opsz24.svg";
                }
                else if(rssi <-80 && rssi>-100){
                    document.getElementById("wifi").src="img/network_wifi_1_bar_FILL0_wght400_GRAD0_opsz24.svg";
                }
                document.getElementById("dbm").innerHTML= "la potencia de la señal es de "+rssi+" dBm";
            }
            else{
                console.log("la respuesta del servidor no fue 200");
            }
        })
        .catch(function(error){
           console.log("no hay conexion con el modulo de cabeza");
        });

}

function main(){
    wifi();
    
    //document.getElementById("video").src="http://192.168.1.120:81/stream";
    for(let p=1; p<3;p++){ // el 4 es el numero de modulos a evaluer
        let abo = AbortSignal.timeout(1000);
        let url = 'http://192.168.1.10'+p+'/ping';
        fetch(url,{signal: abo })
            .then(function(response){
                if (response.ok){
                    if(!document.getElementById("Modulo "+ p)){
                        creartabla();
                    }else{
                        recibirsensores(p);
                        if(document.getElementById("m"+p)==="1"){
                            document.getElementById("m"+p).innerHTML="robot en movimiento";
                        }
                        else{
                            document.getElementById("m"+p).innerHTML="Desplegado";
                        }
                    }
                }
                else{                    
                    //console.log("la respuesta del servidor no fue 200");
                }
            })
            .catch(function(error){
                document.getElementById("m"+p).innerHTML="No desplegado";
                //console.log("no hay conexion con el servidor"+p);
            });
    }    
}

function abrir(){
    var coll = document.getElementsByClassName("collapsible");
    var i;
    for (i = 0; i < coll.length; i++) {
        coll[i].addEventListener("click", function() {
            this.classList.toggle("active");
            var content = this.nextElementSibling;
            if (content.style.display === "block") {
                content.style.display = "none";
            } else {
                content.style.display = "block";
            }
        });
    } 
}
function iniciar(){
    intervalid=setInterval(main,1000);
    document.getElementById("video").src="http://192.168.1.120:81/stream";
}
function parar(){
    clearInterval(intervalid);
}
let intervalid;
document.getElementById("demo").addEventListener("click", creartabla);
document.getElementById("iniciar").addEventListener("click",iniciar);
document.getElementById("parar").addEventListener("click",parar);


setInterval(abrir,10);
