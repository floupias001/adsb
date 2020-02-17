#include <iostream>
#include <complex>
#include <iostream>
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
#include <complex>
#include <immintrin.h>

#define PI 3.14159265359

#include "liste_avion.h"  // qui inclut avion.h

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd.h>

using namespace std;


/* =========================== COULEUR TERMINAL =========================*/

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

void black(){ printf("%s", KNRM); }
void red  (const char* txt){ printf("%s%s%s", KRED, txt, KNRM); }
void green(const char* txt){ printf("%s%s%s", KGRN, txt, KNRM); }


/*   ============================== GET NUM =========================== */
int getnum(char* line, char** real, char** img)
{
	//cout << i << endl;
    	*real = strtok(line, " \n");
	if(*real == NULL) *real = "0";
	//cout << "real :" << real[i] << endl;
	*img = strtok(NULL, "\n");
	if (*img == NULL) *img = "0";
	//if (img[i] != NULL) cout << "imag :" << img[i] << endl;
	return 0;
}

	
/*   ============================== PRODUIT SCALAIRE =========================== */
float produit_scalaire(float *buffer){

	/* AVX
	gather 1 1 1 1 1 1 1 1
	hadd   2 2 0 0 2 2 0 0
	hadd   4 0 0 0 4 0 0 0
	sommer 4*2
	*/
	__attribute__ ((aligned (16))) float ps2[8];
	__m256i vindex = _mm256_set_epi32(0, 1, 3, 4, 14, 15, 18, 19 );
	__m256 v_ps2 = _mm256_i32gather_ps (buffer, vindex , 4);
	/* AVX
	carre des elmts
	somme de 32 elmts
	add  2 2 2 2 2 2 2 2
	add  2 2 2 2 2 2 2 2
	add  4 4 4 4 4 4 4 4
	hadd 0 0 8 8 0 0 8 8
	hadd 0 0 0 16 0 0 0 16 
	sommer 16*2 = 32
	*/
	__m256 v_buf1 = _mm256_loadu_ps(buffer);
	v_buf1 =  _mm256_mul_ps(v_buf1, v_buf1);
	__m256 v_buf2 = _mm256_loadu_ps((buffer + 8));
	v_buf2 =  _mm256_mul_ps(v_buf2, v_buf2);
	__m256 v_buf3 = _mm256_loadu_ps((buffer + 16));
	v_buf3 =  _mm256_mul_ps(v_buf3, v_buf3);
	__m256 v_buf4 = _mm256_loadu_ps((buffer + 24));
	v_buf4 =  _mm256_mul_ps(v_buf4, v_buf4);
	v_buf1 = _mm256_add_ps(v_buf1, v_buf2);
	v_buf3 = _mm256_add_ps(v_buf3, v_buf4);
	v_buf1 = _mm256_add_ps(v_buf1, v_buf3);
	v_buf1 = _mm256_hadd_ps(v_ps2, v_buf1);   // hadd 2 2 8 8 2 2 8 8
	v_buf1 = _mm256_hadd_ps(v_buf1, v_ps2);  // hadd 4 16 0 0 4 16 0 0
	_mm256_store_ps(ps2, v_buf1);

	float somme = ps2[1] + ps2[5];
	float ps = ps2[0] + ps2[4];
	somme = somme*8;
	return (ps/sqrt(somme));

}


/*   ============================== CPRNL =========================== */
int cprNL(float x){

	if (x == 0) return 59;
	else {
		if (x == 87) return 2;
		else {
			if (x < 0) x = -x;
			if ( x > 87) return 1;
			else {

				float c = cos(PI*x/180);

				int i = floor(2*PI/(acos(1-0.00547810463/(c*c)) ));

				return i;
			}
		}
	}
}


/*   ============================== LATITUDE =========================== */
float Latitude(int trame[120]){
	float latref = 44.806884;	//latitude de l'ENSEIRB
	float Dlat = 360/(4*15 - trame[40 + 21]);
	float lat = 0;
	int jlat = 0;
	int ref_Dlat = floor(latref/Dlat);
	for (int q=0; q < 17; q++){
		lat += trame[40+ 22 +q]*pow(2,16-q);
	}
	lat = lat/131072; // divise par 2^17
	jlat = ref_Dlat + floor(0.5 + latref/Dlat -ref_Dlat - lat);
	float latitude = Dlat*(jlat + lat);

	return latitude;
}


/*   ============================== LONGITUDE =========================== */
float Longitude(int trame[120], float latitude){
	float lonref = -0.606629;	//longitude de l'ENSEIRB
	//cout << "cprNL :" << cprNL(latitude) << endl;
	float Dlon;
	int tempNL = cprNL(latitude) - trame[40 + 21];
	if ( tempNL != 0) Dlon = 360/tempNL;
	else Dlon = 360;
	float lon = 0;
	int jlon = 0;
	int ref_Dlon = floor(lonref/Dlon);
	for (int q=0; q < 17; q++){
		lon += trame[40+ 39 +q]*pow(2,16-q);
	}
	lon = lon/131072;
	jlon = ref_Dlon + floor(0.5 + lonref/Dlon -ref_Dlon - lon);
	float longitude = Dlon*(jlon + lon);

	return longitude;
}


/*   ============================== CRC =========================== */
int CRC( int trame[120]){
				
	bitset<25> crc = 0b0000000000000000000000000;
	bitset<25> polynome2 = 0b1111111111111010000001001;
	bitset<25> polynome = 0b1001000000101111111111111;

	for(int q=8; q <33; q++){
		crc[q - 8] = trame[q];
	}

	for (int q=33; q < 120; q++){
		if ( crc[0] == 1){
			crc = crc xor polynome;
		}
		crc = crc >> 1;
		crc[24] = trame[q];
	}

	if ( crc[0] == 1){
		crc = crc xor polynome;	
	}
					
	if (crc == 0x00000000) return 1;
	else return 0;		
}					


/*   ============================== MAIN =========================== */
int main(int argc, char* argv[])
{
	char* buffer_real; //a remplir avec .csv parse test2
	char* buffer_imag;
	//4 ech = 1symb , correspond à 1s
	int trame[120]; //1 trame = 120 symb = 480 ech
	float ps = 0; //produit scalaire
        char line[1024];
	float breal = 0.0;
	float bimag = 0.0;
	int oaci = 0;
	int ftc = 0;
	int altitude = 0;
	float latitude = 0;
	float longitude = 0;
	int caractere[8];
	Liste_Avion *liste_avion;
	liste_avion = new Liste_Avion();
	Avion *plane;
	int nouvel = 0;
	int nbtrame = 0; // nbre elmt/trame/...
	int adsb = 0;
	int bonftc = 0;
	int boncrc = 0;
	int nbtrametotal = 0; // nbre elmt/trame/...
	int adsbtotal = 0;
	int bonftctotal = 0;
	int boncrctotal = 0;
	int c; //getopt
	int verbose = 0;
	int aff_trame = 0;
	int nom_donne = 0;
	int digit_optind = 0;
	int aopt = 0, bopt = 0;
	char *copt = 0, *dopt = 0;
	float ps_min = 0.75;
	static struct option long_options[] = {
	/*   NAME       ARGUMENT           FLAG  SHORTNAME */
	{"verbose", no_argument,       NULL, 'v'},  		// affiche beaucoup de trucs pas tres utiles
	{"trame", no_argument,       NULL, 't'},   		// affiche toutes les trames adsb reçues
	{"produit_scalaire",    required_argument, NULL, 's'}, 	// pour changer la valeur min de la correlation (synchro)
	{"Np",    required_argument, NULL, 'n'}, 		// pour changer le nombre de boucle Np (ie nbre echantillon*200000) // Np = 10 => 0.5 s ?
	{NULL,      0,                 NULL, 0}
	};
	int option_index = 0;
	auto fc = 1090e6;//radio
	auto fe = 8e6;
	auto N = 200000;
	auto Np = 100;

	cout << "============ ADSB ============" << endl;
	// ============== GETOPT ================
	printf("%s",KRED);
	while ((c = getopt_long(argc, argv, "n:s:vt",long_options, &option_index)) != -1) {
		int this_option_optind = optind ? optind : 1;
		switch (c) {
			case 0:
			    printf ("%soption %s%s", long_options[option_index].name, KNRM, KRED);
			    if (optarg)
				printf ("%s with arg %s%s", optarg, KNRM, KRED);
			    printf ("\n");
			    break;
			case 's':
			    dopt = optarg;
		            ps_min = atof(dopt);
			    if ((ps_min > 1) || (ps_min < 0)){
				 ps_min = 0.75;
				 printf("ERREUR : --produit_scalaire ou -s compris entre 0 et 1");
			    	cout <<" ==>  option produit_scalaire : "<< ps_min << endl;
			    } else printf("%soption produit_scalaire : %f%s\n", KNRM, ps_min, KRED);
			    break;
			case 'n':
			    copt = optarg;
		            Np = atoi(copt);
			    if ((Np < 1)){
				 Np = 100;
				 printf("ERREUR : --Np ou -n est entier >1");
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
	printf("%s",KYEL);
	// ================ RADIO =====================
	std::string usrp_addr("type=b100");           // L'adresse de l'USRP est écrite en dur pour l'instant
	uhd::usrp::multi_usrp::sptr usrp;              // Le pointeur vers l'USRP
	usrp = uhd::usrp::multi_usrp::make(usrp_addr); // Initialisation de l'USRP

	usrp->set_rx_rate(fe);                         // Set de la fréquence d'échantillonnage
	usrp->set_rx_freq(fc);                         // Set de la fréquence porteuse
	//usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t("AB:0"));
	usrp->set_rx_antenna("RX2");

	uhd::stream_args_t
	stream_args("fc32");        // Type des données à échantillonner (ici complexes float 64 - 32 bits par voie)
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args); // Pointeur sur les data reçues


	/*
	 * Affichage de quelques paramètres
	 */
	cout << " " << string(50, '-') << endl;
	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

	cout << " Sampling Rate set to: " << usrp->get_rx_rate() << endl;
	cout << " Central Frequency set to: " << usrp->get_rx_freq() << endl;
	cout << " " << string(50, '-') << endl;
	printf("%s%s Ecoute en cours ...\n", KYEL, KNRM);

	/*
     	* Démarrage de l'aquisition
     	*/
	auto start = chrono::high_resolution_clock::now();

	std::vector<std::complex<float>> buffer(N); // Notre buffer à nous dans le programme
	uhd::rx_metadata_t md;                     // Des metadata
	//rx_stream->recv(&buffer.at(0), 1, md);       // Récupere un échantillon (sinon il est nul)

	for (int np = 0; np < Np; np++)
	{
		auto np1 = chrono::high_resolution_clock::now();
		for (int i=1; i<10;i++) if ( np == i*(Np/10)) cout << endl << i << "0%" << endl;
		if (verbose) cout << endl <<   "======== np = " << np <<" =========" << endl;
		// ============== RECEPTION ==================
		int num_rx_samps = 0;                     // Nombre d'echantillons reçus
		while (num_rx_samps < buffer.size())
			num_rx_samps += rx_stream->recv(&buffer.at(num_rx_samps), N - num_rx_samps, md);
		//cout << num_rx_samps << endl; 
		nbtrame = 0;
		adsb = 0;
		bonftc = 0;
		vector<float> buffer_abs; 
		// =============== TRAITEMENT =================


		// =============== cplx => abs() ==============

		/* AVX
		load 8 par 8
		mul real 1 1 1 1 1 1 1 1 
		mul imag 1 1 1 1 1 1 1 1
		add      1 1 1 1 1 1 1 1
		sqrt     1 1 1 1 1 1 1 1
		push_back a la suite
		*/
		if (verbose){
			auto parseur1 = chrono::steady_clock::now();
			int kk = 0;
			while (kk < buffer.size()){
				__attribute__ ((aligned (16))) float breal[8] = {real(buffer[kk + 0]), real(buffer[kk + 1]), real(buffer[kk + 2]), real(buffer[kk + 3]), real(buffer[kk + 4]), real(buffer[kk + 5]), real(buffer[kk + 6]), real(buffer[kk + 7])};
				__attribute__ ((aligned (16))) float bimag[8] = {imag(buffer[kk + 0]), imag(buffer[kk + 1]), imag(buffer[kk + 2]), imag(buffer[kk + 3]), imag(buffer[kk + 4]), imag(buffer[kk + 5]), imag(buffer[kk + 6]), imag(buffer[kk + 7])};
				__m256 v_breal = _mm256_load_ps(breal);
				__m256 v_bimag = _mm256_load_ps(bimag);
				v_breal =  _mm256_mul_ps(v_breal, v_breal);
				v_bimag =  _mm256_mul_ps(v_bimag, v_bimag);
				v_breal =  _mm256_add_ps(v_breal, v_bimag);
				v_breal = _mm256_sqrt_ps(v_breal);
				_mm256_store_ps(breal, v_breal);
				for(int i=0; i <8; i++) buffer_abs.push_back(breal[i]);
				kk += 8;	
			}
			auto parseur2 = chrono::steady_clock::now();

			cout << "\nTemps cplx => abs : " << chrono::duration_cast<chrono::microseconds>(parseur2 - parseur1).count() << " us\n" << endl;
		}else{

			int kk = 0;
			while (kk < buffer.size()){
				__attribute__ ((aligned (16))) float breal[8] = {real(buffer[kk + 0]), real(buffer[kk + 1]), real(buffer[kk + 2]), real(buffer[kk + 3]), real(buffer[kk + 4]), real(buffer[kk + 5]), real(buffer[kk + 6]), real(buffer[kk + 7])};
				__attribute__ ((aligned (16))) float bimag[8] = {imag(buffer[kk + 0]), imag(buffer[kk + 1]), imag(buffer[kk + 2]), imag(buffer[kk + 3]), imag(buffer[kk + 4]), imag(buffer[kk + 5]), imag(buffer[kk + 6]), imag(buffer[kk + 7])};
				__m256 v_breal = _mm256_load_ps(breal);
				__m256 v_bimag = _mm256_load_ps(bimag);
				v_breal =  _mm256_mul_ps(v_breal, v_breal);
				v_bimag =  _mm256_mul_ps(v_bimag, v_bimag);
				v_breal =  _mm256_add_ps(v_breal, v_bimag);
				v_breal = _mm256_sqrt_ps(v_breal);
				_mm256_store_ps(breal, v_breal);
				for(int i=0; i <8; i++) buffer_abs.push_back(breal[i]);
				kk += 8;	
			}

		}
		// ============== decodage ================
		int k=0;
		while ( k <= (buffer_abs.size() - 480)){ //480 = taille trame (ech)
			float s;
			float* addr = buffer_abs.data() + k;
			s = produit_scalaire(addr);

			if (s > ps_min){ 
		
				// -------- on a une trame : ech --------------
				for (int j=0; j < 480; j = j+4){
					if ((buffer_abs[k + j] + buffer_abs[k + j +1]) >= (buffer_abs[k + j +2] + buffer_abs[k + j +3])){
						trame[j/4] = 1;	
					} else {trame[j/4] = 0;}
				}
				// --------- on a une trame demodulee : symb ------------
				oaci = 0;
				ftc = 0;
				altitude = 0;
				latitude = 0;
				longitude = 0;

				if ((trame[8] == 1) &&  (trame[9] == 0) && (trame[10] == 0) && (trame[11] == 0) && (trame[12] == 1)){
					//auto trame1 = chrono::steady_clock::now();
					k+=479;
					// -------------- on a une trame ads-b -----------------

					// calcul @OACI
					for (int q=0; q < 24; q++){
						oaci += trame[16+q]*pow(2,23-q);		
					}

					//calcul CRC
					int crc = CRC(trame);
					if (aff_trame) {
						if (crc) cout << "CRC checked !" << "    ";
						else {
							printf("\n");
							red("====== trame ADSB detectée ======\n");
							printf( "%sp.s. :%f    %s", KRED, s, KNRM);
							printf("%sOACI : %06X%s    ", KRED, oaci, KNRM);
							red("/!/ CRC non checked /!/\n");
							
						}
					}
					if (crc) {
						boncrc++;
						boncrctotal++;
						// ============== avion ======================
						nouvel = (*liste_avion).search(oaci);
						if (nouvel == -1) {
							plane = new Avion(oaci);
							if (verbose) cout << "NOUVEL AVION" << "    ";
						} else {
							if (verbose) cout << "                ";
							*plane = (*liste_avion).find(nouvel);
						}


						//calcul ftc
						for (int q=0; q < 5; q++){
							ftc += trame[40+q]*pow(2,4-q);
						}
						if(aff_trame) cout << "ftc :" << ftc << "    ";

						if ((ftc >0) && (ftc < 5)) {

							// =============== identification =====================
							if(aff_trame) cout << "---- identification ----"<< "   ";
					
							caractere[7] = 0;
							//caractere 1
							for (int q=0; q < 6; q++){
								 caractere[7] += trame[40+ 8 +q]*pow(2,5-q);
							}
							if (caractere[7] < 32) caractere[7] += 65 -1;

							//caractere 2
							caractere[6] = 0;
							for (int q=0; q < 6; q++){
								 caractere[6] += trame[40+ 14 +q]*pow(2,5-q);
							}
							if (caractere[6] < 32) caractere[6] +=  65 -1;

							//caractere 3
							caractere[5] = 0;
							for (int q=0; q < 6; q++){
								 caractere[5] += trame[40+ 20 +q]*pow(2,5-q);
							}
							if (caractere[5] < 32) caractere[5] += 65 -1;

							//caractere 4
							caractere[4] = 0;
							for (int q=0; q < 6; q++){
								 caractere[4] += trame[40+ 26 +q]*pow(2,5-q);
							}
							if (caractere[4] < 32) caractere[4] += 65 -1;

							//caractere 5
							caractere[3] = 0;
							for (int q=0; q < 6; q++){
								 caractere[3] += trame[40+ 32 +q]*pow(2,5-q);
							}
							if (caractere[3] < 32) caractere[3] += 65 -1;

							//caractere 6
							caractere[2] = 0;
							for (int q=0; q < 6; q++){
								 caractere[2] += trame[40+ 38 +q]*pow(2,5-q);
							}
							if (caractere[2] < 32) caractere[2] += 65 -1;

							//caractere 7
							caractere[1] = 0;
							for (int q=0; q < 6; q++){
								 caractere[1] += trame[40+ 44 +q]*pow(2,5-q);
							}
							if (caractere[1] < 32) caractere[1] += 65 -1;

							//caractere 8
							caractere[0] = 0;
							for (int q=0; q < 6; q++){
								 caractere[0] += trame[40+ 50 +q]*pow(2,5-q);
							}
							if (caractere[0] < 32) caractere[0] += 65 -1;

							if (aff_trame) {
								printf("nom : ");
								for(int q=7; q>0; q--) printf("%c", caractere[q]);
								printf("\n");
							}
							plane->set_nom(caractere);

							bonftc++;
							bonftctotal++;
						}else {
							if (ftc < 9){
								// =============== position surface =================
								if(aff_trame) cout << "---- position surface ----" << "    ";

								//calcul latitude et longitude
								longitude = Longitude(trame,latitude = Latitude(trame));
								if (aff_trame){
									cout << "                ";
								 	cout << "latitude  : " << latitude << "    ";
								 	cout << "longitude : " << longitude << endl;
								}
								plane->set_longitude(longitude);
								plane->set_latitude(latitude);
								bonftc++;
								bonftctotal++;
							} else {
								if ((ftc < 23) && (ftc != 19)){

									// =============== position dans l'air ==================
									if(aff_trame) cout << "---- position air ----" << "    ";

									//calcul altitude
									for (int q=0; q < 12; q++){
										if(q > 8) altitude += trame[48+q]*pow(2,10-q-1);
										else {if(q < 8) altitude += trame[48+q]*pow(2,10-q);}
									}
									altitude = altitude*25 - 1000;
									if(aff_trame) cout << "altitude  : " << altitude << " pieds" << "    ";
									plane->set_altitude(altitude);
									//calcul latitude et longitude
									longitude = Longitude(trame, latitude = Latitude(trame));
									if (aff_trame){
								 		cout << "latitude  : " << latitude << "    ";
								 		cout << "longitude : " << longitude << endl;
									}
									plane->set_longitude(longitude);
									plane->set_latitude(latitude);
									bonftc++;
									bonftctotal++;
								} else {
									 if (ftc == 19) {
										// =============== vitesse ==================
										if(aff_trame) cout << "---- vitesse ----" << "         ";
										/*cf https://mode-s.org/decode/adsb.html
										VITESSE : si TYPE == 1 (ground speed)
										1-5 : FTC	6-8 : TYPE	14 : EWsign	15-24 : EW	25 : NSsign	26-35 : NS	37 : VERsign	38-46 : VER
								   		VITESSE : si TYPE == 3 (airspeed)
										1-5 : FTC	6-8 : TYPE									37 : VERsign	38-46 : VER
										*/
										bonftc++;
										bonftctotal++;
										if ((trame[39 + 6] == 0) & (trame[39 + 7] == 0) & (trame[39 + 8] == 1)){
											// Type 1
											// vitesse horizontale
											int vNS = 0;
											for (int q=0; q<10; q++) vNS += trame[39 + 26 + q]*pow(2, 10-q );
											if ( trame[39 + 25] == 1) vNS = 1 - vNS;
											else vNS = vNS - 1;
											int vEW = 0;
											for (int q=0; q<10; q++) vEW += trame[39 + 15 + q]*pow(2, 10-q );
											if ( trame[39 + 25] == 1) vEW = 1 - vEW;
											else vEW = vEW - 1;
											float speed = sqrt(vEW*vEW + vNS*vNS);  // en kt ?? plutot en km/h
											//speed = speed * 1.852;
											plane->set_vit_hor(speed);
											if(aff_trame) cout << "vitesse horizontale : " << speed << " km/h    ";

											// angle
											float angle = atan2(vEW, vNS)*360/(2*PI);
											if(aff_trame) cout << "angle : " << angle << " °" << "    ";
											plane->set_angle(angle);

											// vitesse verticale
											int Vr = 0;
											for (int q=0; q<8; q++) Vr += trame[39 + 38 + q]*pow(2, 8-q );
											Vr = (Vr - 1) *64;
											if (trame[39 + 37] == 1) Vr = -Vr; // en feet/min
											Vr = Vr*0.3048;
											if(aff_trame) cout << "vitesse verticale : " << Vr << " m/min" << endl;
											plane->set_vit_vert(Vr);

										} else {
											if ((trame[39 + 6] == 0) & (trame[39 + 7] == 1) & (trame[39 + 8] == 1)){
												//Type 3
												// vitesse verticale
												int Vr = 0;
												for (int q=0; q<8; q++) Vr += trame[39 + 38 + q]*pow(2, 8-q );
												Vr = (Vr - 1) *64;
												if (trame[39 + 37] == 1) Vr = -Vr; // en feet/min
												if(aff_trame) cout << "vitesse verticale : " << Vr * 0.3048 << " m/min" << endl;
											} // if type 3
										}//if type 1
									}// if ftc 4
								}// if ftc 3
							}// if ftc 2
						} // if ftc 1

						// rajout avion si nouveau
						if (nouvel == -1) (*liste_avion).append(*plane);
						else (*liste_avion).update(nouvel, *plane);
					}//if crc
					adsb++;
					adsbtotal++;

				}// if adsb
				nbtrame++;
				nbtrametotal++;	
			}// if trame
			k++;
		}// while buffer_abs
		auto np2 = chrono::high_resolution_clock::now();
		if (verbose) {
			cout << endl <<  "Temps np :" << chrono::duration_cast<chrono::microseconds>(np2 - np1).count() << "us"<< endl;
			cout << "\nnombre de trames : "<< nbtrame << endl;
			cout << "\nnombre de trames ads-b : "<< adsb << endl;
			cout << "\nnombre de trames ads-b avec bon crc : "<< boncrc << endl;
			cout << "\nnombre de trames ads-b avec bon ftc : "<< bonftc << endl;
		}
	nbtrame = 0;
	adsb = 0;
	bonftc = 0;
	boncrc = 0;
	} //for np


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

	usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
	rx_stream.reset();
	usrp.reset();
	return 0;
}
