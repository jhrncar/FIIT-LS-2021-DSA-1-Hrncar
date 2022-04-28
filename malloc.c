#include<stdio.h>
#include<string.h>
#include<math.h>
void* heap=NULL;

void memory_init(void *ptr, unsigned int size){//ptr je pointer na pole, ktore simuluje nasu pamat. Size je velkost toho pola
    //nastavenie global pointra
    heap = ptr;

    //nastavenie celej pamate na 0
    memset(ptr, 0, size);

    //Memory Header - 1.miesto - velkost pamate, 2.miesto - pointer na prve volne miesto v pamati (12 od zaciatku)
    *((int *) ptr) = size;
    *((void **) ((int *) ptr + 1 )) = (ptr + 16);

    //prvy volny blok
    *((int *) ptr + 3) = size - 20;
    *(((int *) ((char *) ptr + (size - 4 )))) = size - 20;

}

void *memory_alloc(unsigned int size){//size je pocet bajtov ktore chce uzivatel
    
    //najmensi minimalny blok musi byt 16, aby sa do payloadu mestili pointere
    if (size < 16){
        size = 16;
    }

    //ptr_out je vystupny pointer
    //best_fit je pointer na najlepsi blok, do ktoreho budeme alokovat
    void *ptr_out = NULL;
    void *best_fit = *((void **) ((int *) heap + 1)); //best fit sa nastavi na zaciatok spajaneho zoznamu. Ak je na zaciatku NULL, znamena to, ze cela pamat je plna
    if (best_fit == NULL){
        return 0;
    }
    void *test = NULL;
    void *vystup = NULL;
    //best fit algoritmus
    int best= (*((int *)best_fit - 1)); //zoberie si velkost a adresu prveho volneho bloku
    while (1) {
        int velkost = (*((int *)best_fit-1));
        int rozdiel = velkost-size;
        int kontrol = 0;
        if (rozdiel == 0){
            break;
        }
        else if (rozdiel > 0){
            kontrol = 1; //ak kontrol ostane na 0, ziadny z volnych blokov nie je dostatocne velky
            if (rozdiel < best){ //hlada najmensi dostatocne velky blok
                best = rozdiel;
                vystup = best_fit;
            }
        }
        test=(*(void **)best_fit); //prechadza cez explicitny list
        if (test != NULL){
            best_fit = test;
        }
        else {
            if (kontrol == 0){
                return NULL;
            }
            break;
        }
    }
    if (vystup != NULL){
        best_fit = vystup;
    }


    //klon je pointer na najlepsie miesto v pamati, adresu ma najdenu v best_fite. PtrOut je vytupny pointer, ukazuje na zaciatok alokovanej pamate   
    void *klon = ((int *) best_fit - 1 );
    ptr_out = (char *) klon + 4;
    int hlavicka = (*((int *) klon));
    
    //to na co odkazoval treba dat do predosleho volneho
    void **pointer_dalsi = *((void **) ((char *) klon + 4 )); //pointer na dalsi
    void **pointer_predosli = *((void **) ((char *) klon + 4 + 8)); //pointer na predosli;
    if (pointer_predosli != NULL){
        *(pointer_predosli) = pointer_dalsi;
    }
    if (pointer_dalsi != NULL){
        *(((void **) pointer_dalsi + 1)) = pointer_predosli;
    }
    
    //zaplnime symbolicky alokovanu pamat cislom 1 a zapiseme hlavicku a paticku, ktore maju tvar zaporneho cisla velkosti alokovaneho bloku
    memset(ptr_out, 1, size);
    *(((int *) klon)) = size * -1;
    *((int *) ((char *) klon + ( size + 4 ))) = size * -1;


    //rozdelenie navyse miesta v bloku na dalsi volny blok
    if ((hlavicka > (size)) && (hlavicka-size)>=24){
        void *novy_p = ((char *) klon + size + 8);
        int novy_hlavicka = hlavicka - size - 8;
        *((int *) novy_p) = novy_hlavicka;
        *((int *) ((char *) novy_p + 4 + novy_hlavicka)) = novy_hlavicka;

        //pointer na dalsi volny blok
        *( (void **) ((char *) novy_p + 4)) = pointer_dalsi;
        if (pointer_dalsi!=NULL){ 
            *(((void **) pointer_dalsi + 1)) = ((char *) novy_p + 4);
        } 
        

        //pointer na tento volny blok z minuleho
        *( (void **) ((char *) novy_p + 4 + 8)) = pointer_predosli;
        if (pointer_predosli != NULL){
            *(pointer_predosli) = ((char *) novy_p + 4);
        }
        

    }
    //pismeno V v pamati oznacuje Volne miesto, ktore je ale moc male na to, aby bol z neho vytvoreny samostatny blok. 
    //Program berie ohlad na tieto pismena a hlada ich, neskor sa uvolnia vo free, ked sa bude uvolnovat nejaky nim vedlajsi blok
    else if ((hlavicka-size)<24 && ((hlavicka-size)>0)){
        memset(((int *) ((char *) klon + size + 8)),'V',(hlavicka-size));
    }
    
    //upravenie pointra na zaciatok spajaneho zoznamu volnych blokov
    int i = 0;
    while (1){
            if (((((char *) heap + i + 15 )) == (heap + (*((int *) heap)) -1)) ){
                *((void **) ((int *) heap + 1 )) = NULL;
                break;
            }
            else if ((*(((char *) heap + i + 12)) == 'V') && ((*((int *)(((char *) heap + i + 12))) < 0) || ((*((int *)(((char *) heap + i + 12))) > (*((int *) heap)))))){
                i++; //ak to stretne Vcko, ktore ale je na zaciatku nejakeho bloku ktory nie je header, tak ho nechcem. AK je Vcko na zaciatku prazdneho headeru, tak ho chcem
            }
            else if ((*((int *)(((char *) heap + i + 12))) > 0) && ((*((int *)(((char *) heap + i + 12))) < (*((int *) heap))))){
                *((void **) ((int *) heap + 1 )) = ( heap + i + 16 );
                break;
            }
            else{
                i++;
            }
        }
    
    return ptr_out; 
};

int memory_check(void *ptr){ //ptr je uzivatelom zadany pointer na nejake miesto v pamati
    //zadal sa nulovy pointer?
    if (ptr == NULL){
        return 0;
    }
    //ukazuje to prave na zaciatok alokovaneho tela bloku?
    void *blok = ((char *)heap + 12);
    if ((((char *) ptr - 4 ) > ((char *)heap + 11)) && (((char *)ptr) < ((char *)heap + (*((int *) heap)))) ){
        while ((((char *)blok) < ((char *)heap + (*((int *) heap)))) ){
            if ((((char *)blok) == ((char *)ptr - 4) && (*((int *)blok)) < 0 )){
                return 1;
            }
            else {
                blok = ((char *) blok + abs((*((int *)blok))) + 4 + 4);
            }
        }
        return 0;
    }
    else{
        return 0;
    }
}

int memory_free(void *ptr){

    //uvolnenie pamate, vytvorenie noveho bloku ktory ma v sebe nuly a kladnu hlavicku a patu
    int size = (* ( (int *) ((char *) ptr - 4))) * -1;
    memset(((char *) ptr - 4),0,size+8);
    (* ( (int *) ((char *) ptr - 4)))=size;
    (* ( (int *) ((char *) ptr + size)))=size;

    //spajanie susednych volnych blokov do jedneho bloku
    //pred je pointer na predosli volny blok, po je pointer na dalsi volny blok, velkost1 je velkost predosleho bloku, velkost 2 je velkost dalsieho bloku, u je index pre for cyklus
    void *pred = NULL;
    void *po = NULL;
    int velkost1 = 0;
    int velkost2 = 0;
    int u = 0;
    

    //je potom este nejaky blok?
    //V moze byt maximalne za sebou 23 krat, ak by bol 24 uz je to dostatocne velka pamat
    //uvolni vsetky Vcka a prirad ich k nasmu uvolnovanemu bloku
    if ((((((char *) ptr + size + 3))) != (((char *)heap + (*((int *) heap)) - 1))) ) { 
        if ((*((char *)ptr + size + 5) == 'V') && (*((char *)ptr + size + 6) == 'V')){
            for (u = 1; u < 24; u++){ 
                if (*((char *)ptr + size + u + 4 ) != 'V'){
                    break;
                }
            }
            memset(((char *)ptr - 4),0,size + 8 + u );
            (* ( (int *) ((char *) ptr - 4))) = size + u ;
            (* ( (int *) ((char *) ptr + size + u ))) = size + u ;
            size = (* ( (int *) ((char *) ptr - 4)));
            if ((((((char *) ptr + size + 7 + u))) != (heap + (*((int *) heap)) - 1)) ) {
                velkost2 = *(((int *)((char *) ptr + size + u + 4)));
            }
        }
        else {
            velkost2 = *(((int *)((char *) ptr + size + 4)));
        }
    }
    

    //je predtym este nejaky blok?
    //V moze byt maximalne za sebou 23 krat, ak by bol 24 uz je to dostatocne velka pamat
    //uvolni vsetky Vcka a prirad ich k nasmu uvolnovanemu bloku
    if (((((int *) ptr - 4))) != heap){ 
        if (*((char *)ptr - 5) == 'V'){
            for (u = 0; u < 24; u++){
                if (*((char *)ptr - 4 - u - 1) != 'V'){
                    break;
                }
            }
            memset(((char *)ptr - 4 - u),0,size + 8 + u);
            (* ( (int *) ((char *) ptr - 4 - u))) = size + u;
            (* ( (int *) ((char *) ptr + size))) = size + u;
            ptr = ((char *) ptr - u);
            size = (* ( (int *) ((char *) ptr - 4)));
            velkost1 = *(((int *) ptr - 2));

        }
        else {
            velkost1 = *(((int *) ptr - 2));
        }        
    }
    
    //nasleduju 4 pripady spajania bloku podla prezentacie.
    //Pripad 1 = alokovany - nas - alokovany => alokovany - nas - alokovany
    //je tu potrebne prepojit pointre a zaradit do zoznamu volnych blokov
    if ((velkost1 <= 0) && (velkost2 <= 0)){

        int velkost;
        int k=0;
        void **pomoc;
        while (1){ //prepojenie na predosli prvok
            if ((((char *) ptr - k - 12 - 4)) == heap){
                break;
            }
            else if ((*(((char *) ptr - k - 8)) == 'V') && ((*((int *)(((char *) ptr - k - 8))) < 0) || ((*((int *)(((char *) ptr - k - 8))) > (*((int *) heap)))))){
                k++;
            }
            else if ((*((int *)(((char *) ptr - k - 8))) > 0) && (*((int *)(((char *) ptr - k - 8))) < (*((int *) heap)) )){
                velkost = (*((int *) ((char *) ptr - k - 8)));
                *(((void **) ((char *) ptr + 8))) = ((char *) ptr - k - 8 - velkost);
                *(((void **) ((char *) ptr - k - 8 - velkost))) = ptr;
                break;
            }
            else {
                k++;
            }
        }
        
        //prepojenie na dalsi prvok
        k=0;
        while (1){
        if (((((char *) ptr + k + size + 3)) == (((char *)heap + (*((int *) heap)) - 1))) ){ 
            break;
        }
        else if ((*(((char *) ptr + k + size + 4)) == 'V') && ((*((int *)(((char *) ptr + k + size + 4))) < 0) || ((*((int *)(((char *) ptr + k + size + 4))) > (*((int *) heap)))))){
            k++;
        }
        else if ((*((int *)(((char *) ptr + k + size + 4))) > 0) && ((*((int *)(((char *) ptr + k + size + 4))) < (*((int *) heap))))){
            *(((void **) ((char *) ptr + size + 8 + k + 8))) = ptr;
            *(((void **) ((char *) ptr ))) = ((char *) ptr + size + 8 + k);
            break;
        }
        else{
            k++;
            }
        }
        }


    //Pripad 2 = alokovany - nas - prazdny => alokovany - nas - nas
    //je tu potrebne prepojit pointre a zaradit do zoznamu volnych blokov
    if ((velkost1 <= 0) && (velkost2 > 0) && (velkost2 < (*((int *) heap)))){
        pred = *( ((void **) ((char *) ptr + 8 + size + 8))); //pointer z dalsieho na predosli
        po = *( ((void **) ((char *) ptr + 8 + size))); //pointer z dalsieho na dalsi
        
        memset(((char *)ptr - 4),0,4 + size + 4 + 4 + velkost2 + 4);
        *( ( (int *) ((char *) ptr - 4 ))) = velkost2 + size + 8;
        *( ((int *) ((char *) ptr + size + 4 + 4 + velkost2 ))) = velkost2 + size + 8;
        *( ((void **) ((char *) ptr + 8))) = pred;
        if (pred != NULL){
            *((void **)pred) = ptr;
        }
        *( ((void **) ((char *) ptr ))) = po;
        if (po != NULL){
            *((void **)po) = ptr;
        }

    }
    
    //Pripad 3 = prazdny - nas - alokovany => nas - nas - alokovany
    //je tu potrebne prepojit pointre a zaradit do zoznamu volnych blokov
    if ((velkost1 > 0) && (velkost2 <= 0) && (velkost1 < (*((int *) heap)))){
        pred = *( ((void **) ((char *) ptr - 8 - velkost1 + 8))); //pointer z predchadzajuceho na dalsi
        po = *( ((void **) ((char *) ptr - 8 - velkost1))); //pointer z predchadzajuceho na predosli

        memset(((char *) ptr - 8 - velkost1 - 4),0,(velkost1 + size + 16));
        *(( (int *)((char *) ptr - 8 - velkost1 - 4))) = velkost1 + size + 8;
        *(((int *) ((char *) ptr + size))) = velkost1 + size + 8;
        *( ((void **) ((char *) ptr - 8 - velkost1 + 8))) = pred;
        *( ((void **) ((char *) ptr - 8 - velkost1))) = po;

    }


    //pripad 4 - najtazsi pripad = prazdny - nas - prazdny => nas - nas - nas
    //je potrebne tu cely velky blok zaradit na spravne miesto do spajaneho zoznamu
    if ((velkost1 > 0) && (velkost2 > 0) && (velkost2 < (*((int *) heap))) && (velkost1 < (*((int *) heap)))){
        void **dals_dals = *( (void **) ((char *) ptr + size + 8));
        void **predosli = *( (void **) ((char *) ptr - 4 - 4 - velkost1 + 8));
        memset(((char *)ptr - 4 - 4 - velkost1 - 4),0,4 + velkost1 + 4 + 4 + size + 4 + 4 + velkost2 + 4);
        *((int *)((char *)ptr - 4 - 4 - velkost1 - 4)) = velkost1 + 4 + 4 + size + 4 + 4 + velkost2;
        *((int *)((char *)ptr + size + 4 + 4 + velkost2)) = velkost1 + 4 + 4 + size + 4 + 4 + velkost2;
        *((void **)((char *)ptr - 4 - 4 - velkost1)) = dals_dals;
        *((void **)((char *)ptr - 4 - 4 - velkost1 + 8)) = predosli;
        if (dals_dals != NULL){
            *(((void **) dals_dals + 1)) = ((char *)ptr - 4 - 4 - velkost1);
        }

    }



    //upravenie pointra na zaciatok spajaneho zoznamu volnych blokov
    int i = 0;
    while (1){
            if (((((char *) heap + i + 15 )) == (heap + (*((int *) heap)) - 1)) ){
                *((void **) ((int *) heap + 1 )) = NULL;
                break;
            }
            else if ((*(((char *) heap + i + 12)) == 'V') && ((*((int *)(((char *) heap + i + 12))) < 0) || ((*((int *)(((char *) heap + i + 12))) > (*((int *) heap)))))){
                i++; //ak to stretne Vcko, ktore ale je na zaciatku nejakeho bloku ktory nie je header, tak ho nechcem. AK je Vcko na zaciatku prazdneho headeru, tak ho chcem
            }
            else if ((*((int *)(((char *) heap + i + 12))) > 0) && ((*((int *)(((char *) heap + i + 12))) < (*((int *) heap))))){
                *((void **) ((int *) heap + 1 )) = ( heap + i + 16 );
                break;
            }
            else{
                i++;
            }
        }
    
    ptr = NULL;
    return 0;
}


//zaplnenie pamate, otestovanie uvolnenia prveho pola, 2x spojenie podla pripadu 3, spojenie podla pripadu 2, vlozenie V a nasledne ich uvolnenie
/*int main(void){
    char memory[200];
    memory_init(memory,200);
    void *p0 = memory_alloc(15);
    void *p1 = memory_alloc(15);
    void *p2 = memory_alloc(15);
    void *p3 = memory_alloc(15);
    void *p4 = memory_alloc(15);
    
    memory_free(p0); //funkcia zisti, ze pred tymto blokom sa nachadza zaciatok pamate a nepristupuje do nej
    memory_free(p1); //spojenie podla typu 3
    memory_free(p2);
 
    void* p5 = memory_alloc(15);
    void* p6 = memory_alloc(15);
    memory_free(p5);
    p5 = memory_alloc(15);
    void* p7 = memory_alloc(15);
    void* p8 = memory_alloc(15);
    memory_free(p8);
    memory_free(p6); //spojenie podla typu 2 a uvolnenie V za blokom
    



    return 0;
}*/

//test na best fit, na uvolnovnanie Vcok v strede pamate a spojenie blokov podla typu 3
/*int main(void){
    char memory[500];
    memory_init(memory, 500);
 
    void* p0 = memory_alloc(20);
    void* p1 = memory_alloc(30);
    void* p2 = memory_alloc(40);
    void* p3 = memory_alloc(50);
    void* p4 = memory_alloc(35);
    void* p5 = memory_alloc(45);
    void* p6 = memory_alloc(20);
    void* p7 = memory_alloc(25);
    void* p8 = memory_alloc(33);

    memory_free(p0); //volna 20
    memory_free(p3); //volna 50
    memory_free(p5); //volna 45
    memory_free(p7); //volna 25

    void* p9 = memory_alloc(40);//vyhovuje 50 a 45, best fit vyberie 45, vlozi sa 5 Vcok
    void* p10 = memory_alloc(20); //vyhovuje 20 a 25, best fit vyberie rovno 20 a dalej sa ani nepozrie
    memory_free(p6); //uvolni p6, pred ktorou je 5x V,tieto Vcka to zoberie a zakomponuje do noveho volneho bloku, vykona aj spojenie s dalsim prazdnym blokom podla pripadu 2

    return 0;
}*/

//praca s vacsimi cislami, nedostatok miesta v pamati
/*int main(void){
    char memory[5000];
    memory_init(memory,5000);
    void *p0 = memory_alloc(1500);
    void *p1 = memory_alloc(2700);
    void *p2 = memory_alloc(1200);//moc velky, vrati NULL
    memory_free(p1);
    void *p3 = memory_alloc(2000);
    void *p4 = memory_alloc(800);
    memory_free(p0);



    return 0;
}*/

//memory check test, vypisuje ale len 1ky, kvoli prehladnosti
/*int main (void){
    char memory[170];
    memory_init(memory,170);
    void *p0 = memory_alloc(15);
    void *p1 = memory_alloc(15);
    void *p2 = memory_alloc(15);
    void *p3 = memory_alloc(15);
    //void *p4 = memory_alloc(15);
    //memory_free(p2);

    for (int i = 0; i<170; i++){
        int k=memory_check(((char *)heap + i));
        if (k){
            printf("%d\n",k);
        }
    }
    return 0;
}*/

//zakladny test s najvacsimi cislami
/*int main (void){
    char memory[60000];
    memory_init(memory,60000);
    void *p0 = memory_alloc(50000);
    void *p1 = memory_alloc(421);
    void *p2 = memory_alloc(6900);
    memory_free(p1);
    void *p3 = memory_alloc(11);

    memory_free(p2);
    void *p4 = memory_alloc(8000);
    memory_free(p0);


    return 0;
}*/

//len jeden velky blok je v pamati, musi otestovat vsetky hranicne hodnoty, spojenie podla pripadu 1
/*int main(void){
    char memory[100];
    memory_init(memory,100);
    void *p0 = memory_alloc(80);
    memory_free(p0);
    return 0;
}*/