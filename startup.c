/*
 * 	startup.c
 *  
 */
  /* Masker för kontrollbitar */
#define B_E 0x40 /* Enable-signal */
#define B_SELECT 4 /* Välj ASCII-display */
#define B_RW 2 /* 0=Write, 1=Read */
#define B_RS 1 /* 0=Control, 1=Data */

#define GPIOE 0x40021000
#define GPIOE_MODER ((volatile unsigned int*) (GPIOE))
#define GPIOE_OTYPER ((volatile unsigned short*) (GPIOE+0x4))
#define GPIOE_IDRHIGH ((volatile unsigned short*) (GPIOE+0x12))
//#define GPIOE_ODR ((volatile unsigned short*) (GPIOE+0x14))
#define GPIOE_ODRLOW ((volatile unsigned char*) GPIOE+0x14)
#define GPIOE_ODRHIGH ((volatile unsigned char*) (GPIOE+0x15))
#define BARGRAPH ((volatile unsigned short*) (GPIOE_ODR))


#define SYSTICK 0xE000E010
#define STK_CTRL ((volatile unsigned int*) SYSTICK)
#define STK_LOAD ((volatile unsigned int*) (SYSTICK + 0x4))
#define STK_VAL ((volatile unsigned int*) (SYSTICK + 0x8))	
 
//definiera pekare till GPIO_register
#define GPIO_D 0x40020C00
#define GPIO_MODER ((volatile unsigned int *) (GPIO_D))
#define GPIO_OTYPER ((volatile unsigned short *) (GPIO_D+0x4))
#define GPIO_PUPDR ((volatile unsigned int *) (GPIO_D+0xC))
#define GPIO_IDR_LOW ((volatile unsigned char *) (GPIO_D+0x10))
#define GPIO_IDR_HIGH ((volatile unsigned char *) (GPIO_D+0x11))
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14))
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_D+0x15)) 

 
 #define SIMULATOR
 
 
__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");		/* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");					/* call main */
__asm__ volatile(".L1: B .L1\n");				/* never return */
}


__attribute((naked))
void graphic_initialize(void)
{
	__asm volatile (" .HWORD 0xDFF0\n");
	__asm volatile (" BX LR\n");
}

__attribute((naked))
void graphic_pixel_set(int x, int y)
{
	__asm volatile (" .HWORD 0xDFF2\n");
	__asm volatile (" BX LR\n");
}

__attribute((naked))
void graphic_clear_screen(void)
{
	__asm volatile (" .HWORD 0xDFF1\n");
	__asm volatile (" BX LR\n");
}

__attribute((naked))
void graphic_pixel_clear(int x, int y)
{
	__asm volatile (" .HWORD 0xDFF3\n");
	__asm volatile (" BX LR\n");
}


//////////ASCII

char ascii_read_controller( void )

{
char c;
ascii_ctrl_bit_set( B_E );
delay_250ns();
delay_250ns();
c = *GPIOE_IDRHIGH;
ascii_ctrl_bit_clear( B_E );
return c; }

void ascii_write_controller( char c )
{
ascii_ctrl_bit_set( B_E );
*GPIOE_ODRHIGH = c;
delay_250ns();
ascii_ctrl_bit_clear( B_E );
}



void ascii_write_data(unsigned char data)
{ ascii_ctrl_bit_set(B_RS);
  ascii_ctrl_bit_clear(B_RW);
  ascii_write_controller(data);
	}
	
void ascii_write_cmd(unsigned char command)
{ ascii_ctrl_bit_clear(B_RS);
  ascii_ctrl_bit_clear(B_RS);
  ascii_write_controller(command);
	}
void ascii_ctrl_bit_set( char x )
{ /* x: bitmask bits are 1 to set */
char c;
c = *GPIOE_ODRLOW;
*GPIOE_ODRLOW = B_SELECT | x | c;
}

void ascii_ctrl_bit_clear( char x )
{ /* x: bitmask bits are 1 to clear */
char c;
c = *GPIOE_ODRLOW;
c = c & ~x;
*GPIOE_ODRLOW = B_SELECT | c;
}

char ascii_read_status( void )
{
char c;
*GPIOE_MODER = 0x00005555;
ascii_ctrl_bit_set( B_RW );
ascii_ctrl_bit_clear( B_RS );
c = ascii_read_controller();
*GPIOE_MODER = 0x55555555;
return c;
}

char ascii_read_data( void )
{
char c;
*GPIOE_MODER = 0x00005555;
ascii_ctrl_bit_set( B_RW );
ascii_ctrl_bit_set( B_RS );
c = ascii_read_controller();
*GPIOE_MODER = 0x55555555;
return c;
}

void init_app_ascii(void)
{ *GPIOE_MODER= 0x55555555;
  *GPIOE_OTYPER |0x11110000;
	}

void ascii_init(void)
{  
	
	ascii_write_cmd(0x38); ///function set
	delay_micro(40);
	ascii_write_cmd(0xE); //display control
	delay_micro(40);
	ascii_write_cmd(0x1); ///clear display
	 delay_milli(2);
	ascii_write_cmd(0xA); //ENTRY MODE SET
	 delay_micro(40);  
 
	}
	

void ascii_write_char(char c)
{ while((*STK_CTRL & 0x10000)==0); //vänta tills COUNTFLAG blir 1
  delay_micro(8);
  ascii_write_data(c); 
   delay_micro(8);  //OSÄKER PÅ LÄNGDEN AV FÖRDRÖJNINGEN
	}
	
void ascii_gotoxy(int row, int column)
{  int address;
   address=row-1;
   if (column==2)
   { address= address+0x40;}
  ascii_write_cmd( address|0x80); 
}

//////////KEYBOARD

void ActivateRow(unsigned int row)
{  
/* Aktivera angiven rad hos tangentbordet, eller deaktivera samtliga */
	switch( row ){

	case 1: *GPIO_ODR_HIGH = 0x10; break;
	case 2: *GPIO_ODR_HIGH = 0x20; break;
	case 3: *GPIO_ODR_HIGH = 0x40; break;
	case 4: *GPIO_ODR_HIGH = 0x80; break;
	case 0: *GPIO_ODR_HIGH = 0x00; break; }
	
	
}

int ReadColumn(void)
{ 
	unsigned char c;
	c=*GPIO_IDR_HIGH; // Vi plockar enbart ut de fyra bitar som följer adressen GPIO_IDR_HIGH. Dessa innehåller de insignaler vi vill ha. Därför är c satt som en char
	if (c & 0x8) return 4; 
	if (c & 0x4) return 3; 
	if (c & 0x2) return 2; 
	if (c & 0x1) return 1; 
	return 0; 
	
	}
//---------------------------------------------------------------------

unsigned char keyb(void)
{ 
	unsigned char keyValue[]={1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
	int row,col;
	
	for(row=1;row<=4;row++)
	{ 
		ActivateRow(row);
		col=ReadColumn();
		
		if (col !=0) 	
			{ ActivateRow(0); //deaktivera tangenbordet
				return keyValue[4*(row-1)+(col-1)];
			} 
	
    }
	ActivateRow(0);
		return 0xFF;
}

////////////////////////

///////////////////STRUCTS

  typedef struct{
	 char x,y
 } POINT,*PPOINT;
 
 
 #define MAX_POINTS 30
 
 typedef struct{
	 int numpoints;
	 int sizex;
	 int sizey;
	 POINT px[ MAX_POINTS ];
 } GEOMETRY,*PGEOMETRY;
 
 typedef struct tObj{
	 PGEOMETRY geo;  
	 int dirx,diry;
	 int posx,posy;
	 void (*draw)(struct tObj *);
	 void (*clear)(struct tObj *);
	 void (*move)(struct tObj *);
	 void (*set_speed)(struct tObj *,int,int); 
 }OBJECT,*POBJECT;
 
 
 

void delay_250ns(void)
  { *STK_CTRL=0x0;
	*STK_LOAD= 168/4-1;
	*STK_VAL=0x0;
	*STK_CTRL=0x5;
	
	
	while((*STK_CTRL & 0x10000)==0);
	
	  }
	  
void delay_micro(unsigned int us)
{
#ifdef SIMULATOR
us = us / 1000;
us++;
#endif
while( us > 0 )
{
delay_250ns();
delay_250ns();
delay_250ns();
delay_250ns();
us--;
}
}
	  
void delay_milli(int millis)
{  
    #ifdef SIMULATOR
	millis=millis/1000;
	millis++;
    #endif;
	
	delay_micro(1000*millis);
	
	
	}



////////////////////////OBJECT FUNCTIONS

void draw_ballobject(POBJECT o){
	
	
	int numberofpoints=o->geo->numpoints;
	
	for (int i=0; i<=numberofpoints ;i++){
		int x= o->posx + o->geo->px[i].x;
		int y= o->posy+ o->geo->px[i].y;
		graphic_pixel_set(x,y);
	}

	
}

void draw_paddleobject(POBJECT p){
	int numberofpoints=p->geo->numpoints;
	
	for (int i=0; i<=numberofpoints ;i++){
		int x= p->posx + p->geo->px[i].x;
		int y= p->posy+ p->geo->px[i].y;
		graphic_pixel_set(x,y);
	}
	
}


void clear_ballobject(POBJECT o){
		int numberofpoints=o->geo->numpoints;
	
	for (int i=0; i<=numberofpoints ;i++){
		int x= o->posx + o->geo->px[i].x;
		int y= o->posy+ o->geo->px[i].y;
		graphic_pixel_clear(x,y);
	}
}

void clear_paddleobject(POBJECT p){
	int numberofpoints=p->geo->numpoints;
	
	for (int i=0; i<=numberofpoints ;i++){
		int x= p->posx + p->geo->px[i].x;
		int y= p->posy+ p->geo->px[i].y;
		graphic_pixel_clear(x,y);
	}
	
}

void set_ballobject_speed(POBJECT o,int speedx, int speedy){
	o->dirx=speedx;
	o->diry=speedy;
	
}

void set_paddleobject_speed(POBJECT p,int speedx, int speedy){
	p->dirx=speedx;
	p->diry=speedy;
	
}


void move_ballobject(POBJECT o){
	clear_ballobject(o);
	
	o->posx = o->posx + o->dirx;
	
	if (o->posx < 1){
		o->dirx=-(o->dirx);
	}

	
	o->posy=o->posy+o->diry;
	
	if (o->posy-o->geo->sizey < 1){
		o->diry=-(o->diry);
	}
	if ((o->posy + o->geo->sizey)>64){
		o->diry=-(o->diry);
	}
	
	draw_ballobject(o);
	
}

void move_paddleobject(POBJECT o){
	clear_paddleobject(o);
	

	
	o->posy=o->posy+o->diry;
	
	if (o->posy < 1){
		o->diry=0;
	}
	if ((o->posx + o->geo->sizex)>64){
		o->diry=-0;
	}
	
	draw_paddleobject(o);
	
}

///////////////////

void init_app(void)
{  *GPIO_MODER=0x55005555;
   *GPIO_PUPDR=0x00AA0000;
   *GPIO_OTYPER=0x0000;
   
   //Har inte satt hastigheten än!
	
	 }


	
GEOMETRY ball_geometry = {
    12,
    4, 4,
    {
	    {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2},
        {1, 3}, {2, 0}, {2, 1}, {2, 2}, {2, 3},
        {3, 1}, {3, 2}
    }
};

GEOMETRY paddle_geometry={
	24,
	5,9,
	 { {0,0}, {1,0}, {2,0},
        {3,0}, {4,0}, {4,1}, 
		{4,2}, {4,3}, {4,4},
        {4,5}, {4,6}, {4,7}, 
		{4,8}, {3,8}, {2,8}, 
		{1,8}, {0,8}, {0,7}, 
		{0,6}, {0,5}, {0,4}, 
		{0,3}, {0,2}, {0,1} }
};

 OBJECT ball_object = {
    &ball_geometry,
    1, 1,
    1, 1,
    draw_ballobject,
    clear_ballobject,
    move_ballobject,
    set_ballobject_speed
};

OBJECT paddle1_object ={
	&paddle_geometry,
	1,1,
	115,32,
	draw_paddleobject,
    clear_paddleobject,
    move_paddleobject,
    set_paddleobject_speed
};


int objects_contact(POBJECT ball,POBJECT paddle1)

{ 
	int toppadel,bottompadel,topball,bottomball;  // nu kontrollerar vi i y-led
	toppadel=paddle1->posy; //-ball->geo->sizey;
	bottompadel=toppadel+paddle1->geo->sizey+ball->geo->sizey;
	topball=ball->posy;
	bottomball=topball+ball->geo->sizey; 
	
	 int condition1= (ball->posx+ball->geo->sizex) == (paddle1->posx);
     int condition2=(toppadel<topball && bottompadel>bottomball);
	 int condition3= ((ball->posx+ball->geo->sizex) == (paddle1->posx));
	
	if (condition1 && condition2)
		return 1;
		else 
		return 0; 
	
		  
	}

void writePlayerStats(char player1, char player2)
{
	
	char *t;
	char test1[]="William";
	char test2[]="Oskar";
	test1[10]= "player1";
	
	init_app_ascii();
	ascii_init();
	ascii_gotoxy(1,1);
	
	t=test1;
	
	while(*t)
		ascii_write_char(*t++);
		
	ascii_write_char(player1);
	ascii_gotoxy(1,2);
	t=test2;
	while(*t)
		ascii_write_char(*t++);
	ascii_write_char(player2);
	init_app();
	
}
	 
	 
  int main()
{
	

	
	
//// ASCIIkod ovan!
 init_app();
 unsigned short s;

 POBJECT ball = &ball_object;
 POBJECT paddle1 = &paddle1_object;
 graphic_initialize();
 graphic_clear_screen();
 ball->set_speed( ball, 3, 1);
 
 while( 1 )
 {
	 
	 
 ball->move( ball );
 ball->draw(ball);
 paddle1->move( paddle1 );

 
 s = keyb();
 
 if (s ==9 && paddle1->posy <50 )
 paddle1->set_speed( paddle1, 0, -1);
 else if (s ==3 && paddle1->posy >10 )
 paddle1->set_speed( paddle1, 0, 1);
 else if (s==6)
 { ball->clear(ball);
	 ball->posx=3;
 ball->posy=16;
 ball->set_speed(ball,-1,2);}
 else
 paddle1->set_speed( paddle1, 0, 0);

 
 if( objects_contact( ball, paddle1 ))
 {
 ball->clear(ball);
 ball->dirx = - ball->dirx;
 ball->draw(ball);

 }
 
 if (ball->posx > 128)
 {
	 ball->clear(ball);
	 ball->set_speed(ball,0,0); }
  delay_milli(40);
 	writePlayerStats("2","2");
 }

 
 }

		









