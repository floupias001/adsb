#include <iostream>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <math.h>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <getopt.h>
#include <immintrin.h>

#include "liste_avion.h"  // qui inclut avion.h
#include "radio.h"
#include "detecteur.h"
#include "detecteur8par8.h"
#include "decodage.h"
#include "abs.h"
#include "couleur.h"

using namespace std;

/*   ============================== GET NUM =========================== */
int getnum(char* line, char** real, char** img)
{
    *real = strtok(line, " \n");
	if(*real == NULL) *real = "0";
	*img = strtok(NULL, "\n");
	if (*img == NULL) *img = "0";
	return 0;
}			


/*   ============================== MAIN =========================== */
/*
	4 ech = 1 symb
	1 trame = 120 symb = 480 ech
*/
int main(int argc, char* argv[])
{
	char* buffer_real;
	char* buffer_imag;
	int trame[120];
    char line[1024];
	float breal = 0.0;
	float bimag = 0.0;
	Liste_Avion *liste_avion;
	liste_avion = new Liste_Avion();

	int nbtrame = 0; // nbre trame ...
	int nbtrametotal = 0;
	int adsbtotal = 0;
	int bonftctotal = 0;
	int boncrctotal = 0;
	int adsb = 0;
	int bonftc = 0;
	int boncrc = 0;

	int c; //getopt
	int verbose = 0;
	int aff_trame = 0;
	char* nom_fichier = "buffers_test.c";
	int fichier = 0;
	int digit_optind = 0;
	float ps_min = 0.75;
	int Np = 100;
	int huit = 0;
	static struct option long_options[] = {
	/*   NAME       ARGUMENT           FLAG  SHORTNAME */
	{"verbose", no_argument,       NULL, 'v'},  		// affiche temps sur chaque boucle Np + cplx => abs
	{"trame", no_argument,       NULL, 't'},   		// affiche toutes les trames adsb reçues
	{"produit_scalaire",    required_argument, NULL, 's'}, 	// pour changer la valeur min de la correlation (synchro)
	{"np",    required_argument, NULL, 'n'}, 		// pour changer le nombre de boucle Np (ie nbre echantillon*200000) // Np = 10 => 0.5 s ?
	{"huit", no_argument, NULL, '8'},  // detecteur8par8
	{"fichier", required_argument,     NULL, 'f'},
	{NULL,      0,                 NULL, 0}
	};
	int option_index = 0;

	Detecteur* detecteur = new Detecteur();
	Detecteur8par8* detecteur8par8 = new Detecteur8par8();
	Radio* radio = new Radio();
	vector<complex<float> > buffer(200000); // Notre buffer à nous dans le programme
	vector<complex<float> > buffer_fichier;

	cout << "==================================== ADSB ====================================" << endl;
	// ============== GETOPT ================
	printf("%s",KRED);
	while ((c = getopt_long(argc, argv, "f:n:s:vt8",long_options, &option_index)) != -1) {
		int this_option_optind = optind ? optind : 1;
		switch (c) {
			case 0:
			    printf ("%soption %s%s", long_options[option_index].name, KNRM, KRED);
			    if (optarg)
				printf ("%s with arg %s%s", optarg, KNRM, KRED);
			    printf ("\n");
			    break;
			case 's':
		        ps_min = atof(optarg);
			    if ((ps_min > 1) || (ps_min < 0)){
				 ps_min = 0.75;
				 printf("erreur : --produit_scalaire ou -s compris entre 0 et 1");
			    	cout <<" ==>  option produit_scalaire : "<< ps_min << endl;
			    } else printf("%soption produit_scalaire : %f%s\n", KNRM, ps_min, KRED);
			    break;
			case 'n':
		        Np = atoi(optarg);
			    if ((Np < 1)){
				 Np = 100;
				 printf("erreur : --Np ou -n est entier >1");
			    	 cout <<" ==>  option Np : "<< Np << endl;
			    } else printf("%soption Np : %d%s\n", KNRM, Np, KRED);
			    break;
			case 'v':
			    verbose = 1;
				printf("%soption verbose%s\n", KNRM, KRED);
			    break;
			case 't':
			    aff_trame = 1;
				printf("%soption trame%s\n", KNRM, KRED);
			    break;
			case 'f':
			    fichier = 1;
				nom_fichier = optarg;
				printf("%soption fichier :%s%s\n",KNRM, nom_fichier, KRED);
			    break;
			case '8' :
				huit = 1;
				printf("%soption huit%s\n",KNRM, KRED);
			case '?':
			    break;
			default:
			    printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}
	if (optind < argc) {
		printf ("non-option ARGV-elements: ");
	while (optind < argc)
	    printf ("%s ", argv[optind++]);
	printf ("\n");
	}
	printf("%s",KNRM);
	cout << endl;




	//=============== INITIALISATION RADIO ================
	if (!fichier){
		radio->initialize(); 
		cout << "Temps estimé : " << Np*5/200 << " s   soit " << (float)Np*5/200/60 << " min"<< endl << endl; 
		printf("Ecoute en cours ...\n");
	}
	//=============== INITIALISATION FICHIER ================
	if (fichier){
		Np = 1;
		FILE* stream;
		 stream = fopen(nom_fichier, "r");
		if (stream == NULL){
			printf("%s", KRED);
			cout << "erreur fopen => nom_fichier = ";
			nom_fichier = "buffers_test.c";
			cout << nom_fichier << endl;
			printf("%s", KNRM);
			stream = fopen(nom_fichier, "r");
			if (stream == NULL) return 1;
		}
		while ( fgets(line, 1024, stream)){
			getnum(line, &buffer_real, &buffer_imag);
			breal = strtof(buffer_real, NULL);  //conv char* => float
			bimag = strtof(buffer_imag, NULL);
			buffer_fichier.push_back(complex<float>(breal, bimag));
		}
		fclose(stream);
	}


	auto start = chrono::high_resolution_clock::now();


	for (int np = 0; np < Np; np++)
	{
		auto np1 = chrono::high_resolution_clock::now();

		if (!fichier){
			for (int i=1; i<10;i++) if ( np == i*(Np/10)) cout << endl << i << "0%" << endl;
			if (verbose) cout << endl <<   "======== np = " << np <<" ========" << endl;
		}
		// ============== RECEPTION RADIO ==================
		if (!fichier) radio->reception(&buffer);

		// =============== TRAITEMENT ================

		// =============== cplx => abs() ==============
		vector<float> buffer_abs; 
		if (fichier) cplx2abs(verbose, &buffer_fichier, &buffer_abs);
		else cplx2abs(verbose, &buffer, &buffer_abs);

		// ============== detection & decodage ================
		int k=0;
		while ( k <= (buffer_abs.size() - 480)){ //480 = taille trame (ech)
			//cout << "k :" << k << endl;
			float s;
			float* addr = buffer_abs.data() + k;

			if (!huit){
				s = detecteur->detection(addr);
				if (s > ps_min){ 
					// -------- on a une trame : ech --------------
					for (int j=0; j < 480; j = j+4){
						if ((buffer_abs[k + j] + buffer_abs[k + j +1]) >= (buffer_abs[k + j +2] + buffer_abs[k + j +3])){
							trame[j/4] = 1;	
						} else {trame[j/4] = 0;}
					}

					// --------- on a une trame demodulee : symb ------------
					if ((trame[8] == 1) &&  (trame[9] == 0) && (trame[10] == 0) && (trame[11] == 0) && (trame[12] == 1)){
						// -------------- on a une trame ads-b -----------------
						k+=479;
						Decodage* decode = new Decodage();
						decode->decodage(s, verbose, aff_trame, trame, liste_avion);
						adsb ++;
						bonftc += decode->get_bonftc();
						boncrc += decode->get_boncrc();
					}

					nbtrame++;
					nbtrametotal++;	
				}
			} else {
				__attribute__ ((aligned (16))) float s8[8];
				detecteur8par8->detection(s8, addr);
				for (int kk=0; kk < 8; kk++){
					if (*(s8 + kk) > ps_min){ 
						// -------- on a une trame : ech --------------
						for (int j=0; j < 480; j = j+4){
							if ((buffer_abs[k + kk + j] + buffer_abs[k + kk + j +1]) >= (buffer_abs[k + kk + j +2] + buffer_abs[k + kk + j +3])){
								trame[j/4] = 1;	
							} else {trame[j/4] = 0;}
						}
						// --------- on a une trame demodulee : symb ------------
						if ((trame[8] == 1) &&  (trame[9] == 0) && (trame[10] == 0) && (trame[11] == 0) && (trame[12] == 1)){
							// -------------- on a une trame ads-b -----------------
							Decodage* decode = new Decodage();
							decode->decodage(*(s8 + kk), verbose,  aff_trame, trame, liste_avion);
							k+=471;
							kk = 8;
							adsb ++;
							bonftc += decode->get_bonftc();
							boncrc += decode->get_boncrc();
						}

						nbtrame++;
						nbtrametotal++;	
					}
				}
				k +=8;
			}
			k++;
		}
		// ============== affichage resultats partiels ================
		auto np2 = chrono::high_resolution_clock::now();

		if (verbose && !fichier) {
			cout << endl <<  "Temps np :" << chrono::duration_cast<chrono::microseconds>(np2 - np1).count() << "us"<< endl;
			cout << "\nnombre de trames : "<< nbtrame << endl;
			cout << "\nnombre de trames ads-b : "<< adsb << endl;
			cout << "\nnombre de trames ads-b avec bon crc : "<< boncrc << endl;
			cout << "\nnombre de trames ads-b avec bon ftc : "<< bonftc << endl;
		}

		adsbtotal += adsb;
		boncrctotal += boncrc;
		bonftctotal += bonftc;
		nbtrame = 0;
		adsb = 0;
		bonftc = 0;
		boncrc = 0;
	}


	// =============== AFFICHAGE FINAL =====================
	printf("\n================================================================\n");

	cout << "liste des avions :" << endl;
	(*liste_avion).print();

	auto end = chrono::high_resolution_clock::now();

	cout << "\nnombre total de trames : "<< nbtrametotal << endl;
	cout << "\nnombre total de trames ads-b : "<< adsbtotal << endl;
	cout << "\nnombre total de trames ads-b avec bon crc : "<< boncrctotal << endl;
	cout << "\nnombre total de trames ads-b avec bon ftc : "<< bonftctotal << endl;

	cout << "\n\nTemps total : " << chrono::duration_cast<chrono::seconds>(end - start).count() << " s" << endl;
 	cout << "- precision: ";
    	typedef typename std::chrono::high_resolution_clock::period P;// type of time unit
   	if (ratio_less_equal<P,nano>::value) {
      		// convert to and print as nanoseconds
       		typedef typename ratio_multiply<P,giga>::type TT;
       		cout << fixed << double(TT::num)/TT::den << " ns" << endl;
    	}


	

	if (!fichier) radio->reset();



	return 0;
}
