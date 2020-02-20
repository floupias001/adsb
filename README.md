# adsb

cd /build   
cmake ..    
make    
./adsb {options}    

options :       
          -f --fichier "nom-fichier"            => utiliser les donnees d'un fichier (default : "buffers_test.c"), desactive la radio   
          -n --np {int}                        => change le temps de detection (default : Np = 1000 => 500s)    
          -s --produit_scalaire {float}         => change le seuil de detection (default : 0.75)    
          -v --verbose                          => affichage pour debug 
          -t --trame                              => desactive affichage trame recues   
          -a --fc {float}                         => change la frequence de la porteuse (default : 1090e6)  
          -b --fe {float}                        => change la frequence d'echantillonnage (default : 4e6)   
          -8 --huit                               => detection differente   
        

carte :         
google map => vos adresses => creer une carte => importer => donner le .kml     