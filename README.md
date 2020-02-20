# adsb

cd /build   
cmake ..    
make    
./adsb {options}    

options :       
          -f ou --fichier "nom-fichier"              => utiliser les donnees d'un fichier (default : "buffers_test.c"), desactive la radio   
          -n ou --np {int}                           => change le temps de detection (default : Np = 1000 => 500s)    
          -s ou --produit_scalaire {float}           => change le seuil de detection (default : 0.75)    
          -v ou --verbose                            => affichage pour debug 
          -t ou --trame                              => desactive affichage trame recues   
          -a ou --fc {float}                         => change la frequence de la porteuse (default : 1090e6)  
          -b ou --fe {float}                         => change la frequence d'echantillonnage (default : 4e6)   
          -8 ou --huit                               => detection plus rapide  


carte :         
google map => vos adresses => creer une carte => importer => donner le .kml     