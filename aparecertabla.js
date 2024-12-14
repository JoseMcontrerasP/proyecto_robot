function numids(){
    const idnode    =   document.getElementById("bloquesitos");
    let nextid      =   idnode.lastElementChild.id;
    let numero      =   nextid[7];
    numero++;
    return numero;
}
function nameid(str,index,char){
    const array = str.split('');
    array[index] = char;
    return array.join('');
}

function creartabla(){
    const node  =   document.getElementById("Modulo 1");
    const clone =   node.cloneNode(true);
    let numeroid = numids();
    clone.id="Modulo " + numeroid; 

    document.getElementById("bloquesitos").appendChild(clone);
    document.getElementById(clone.id).children[0].innerHTML="Modulo "+(numeroid);

    let medio = document.getElementById(clone.id).children;
    let ztable = medio[1].children[0].children;

    for(let i = 0;i<(ztable.length);i++){   
        let inter = ztable[i].children;
        for(let m = 0; m<(inter.length);m++){
            if(inter[m].children[1].nodeName == "TABLE"){
                let row = inter[m].children[1].children[0].children[1].children;
                //console.log(row);
                for(let r=0;r<row.length;r++){
                    let tieneid = row[r].hasAttribute("id");
                    //console.log(row[r]);
                    if (typeof row[r].id === "string" && row[r].id.length !== 0) {
                        row[r].id  = nameid(row[r].id,6,numeroid);
                        row[r].innerHTML   =   "0";
                    }
                }
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
