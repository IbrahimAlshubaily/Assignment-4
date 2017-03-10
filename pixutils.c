#include "pixutils.h"
#include "bmp/bmp.h"
//Ibrahim Alshuabily
//private methods -> make static
static pixMap* pixMap_init();
static pixMap* pixMap_copy(pixMap *p);

//plugin methods <-new for Assignment 4;
static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);

static pixMap* pixMap_init(){
	pixMap *p=malloc(sizeof(pixMap));
	p->pixArray_overlay=0;
	return p;
}	

void pixMap_destroy (pixMap **p){
	if(!p || !*p) return;
	pixMap *this_p=*p;
	if(this_p->pixArray_overlay)
	 free(this_p->pixArray_overlay);
	if(this_p->image)free(this_p->image);	
	free(this_p);
	this_p=0;
}
	
pixMap *pixMap_read(char *filename){
	pixMap *p=pixMap_init();
 int error;
 if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 0;
	}
	p->pixArray_overlay=malloc(p->imageHeight*sizeof(rgba*));
	p->pixArray_overlay[0]=(rgba*) p->image;
	for(int i=1;i<p->imageHeight;i++){
		p->pixArray_overlay[i]=p->pixArray_overlay[i-1]+p->imageWidth;
	}
	return p;
}
int pixMap_write(pixMap *p,char *filename){
	int error=0;
 if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 1;
	}
	return 0;
}	 

pixMap *pixMap_copy(pixMap *p){
	pixMap *new=pixMap_init();
	*new=*p;
	new->image=malloc(new->imageHeight*new->imageWidth*sizeof(rgba));
	memcpy(new->image,p->image,p->imageHeight*p->imageWidth*sizeof(rgba));	
	new->pixArray_overlay=malloc(new->imageHeight*sizeof(void*));
	new->pixArray_overlay[0]=(rgba*) new->image;
	for(int i=1;i<new->imageHeight;i++){
  new->pixArray_overlay[i]=new->pixArray_overlay[i-1]+new->imageWidth;
	}	
	return new;
}

	
void pixMap_apply_plugin(pixMap *p,plugin *plug){
	pixMap *copy=pixMap_copy(p);
	for(int i=0;i<p->imageHeight;i++){
	 for(int j=0;j<p->imageWidth;j++){
			plug->function(p,copy,i,j,plug->data);
		}
	}
	pixMap_destroy(&copy);	 
}

int pixMap_write_bmp16(pixMap *p,char *filename){

 BMP16map *bmp16=BMP16map_init(p->imageHeight,p->imageWidth,0,5,6,5); //initialize the bmp type
 if(!bmp16) return 1;


	//bmp16->pixArray[i][j] is 2-d array for bmp files. It is analogous to the one for our png file pixMaps except that it is 16 bits

 for(int i = 0; i < p->imageHeight; i++) {
 		//However pixMap and BMP16_map are "upside down" relative to each other
 		for (int j = 0; j < p->imageWidth; j++) {
 			//need to flip one of the the row indices when copying
 			bmp16->pixArray[ (p->imageHeight-1) -i][j] =
 				  (p->pixArray_overlay[i][j].r >>3)  <<11 | (p->pixArray_overlay[i][j].g >>2)  <<5
				| (p->pixArray_overlay[i][j].b >>3)       | (p->pixArray_overlay[i][j].a >>8)  <<16;
 		}
 	}
 BMP16map_write(bmp16,filename);
 BMP16map_destroy(&bmp16);
 return 0;
}
void plugin_destroy(plugin **plug){

	/*example:
	   if(!p || !*p) return;
		pixMap *this_p=*p;
		if(this_p->pixArray_overlay)
	 	 free(this_p->pixArray_overlay);
		if(this_p->image)free(this_p->image);
		free(this_p);
		this_p=0;
	 */

	//valgrind --leak-check=yes
	//valgrind --leak-check=full --show-leak-kinds=all
  //free the allocated memory and set *plug to zero (NULL)
	if(!plug || !*plug) return;
		if((*plug)->data)
			free((*plug)->data);
		free(*plug);
		*plug = 0;
}
plugin *plugin_parse(char *argv[] ,int *iptr){
	//malloc new plugin
	plugin *new=malloc(sizeof(plugin));
	new->function=0;
	new->data=0;
	int i=*iptr;
	if(!strcmp(argv[i]+2,"rotate")){

		new->function=rotate;
		  new->data=malloc(2 *sizeof(float));

		  float *sc=(float*) new->data;
		  int i=*iptr;
		  float theta=atof(argv[i+1]);

		  sc[0]=sin(degreesToRadians(-theta));
			 sc[1]=cos(degreesToRadians(-theta));
				//code goes here
				*iptr=i+2;
				return new;
	}	
	if(!strcmp(argv[i]+2,"convolution")){
				//code goes here
		//normalize and make sure it does not add up to 0 <- the check is in the convolution method
		new->function = convolution;

		new->data = malloc(sizeof(int)*9);
		int (*theData) [3] = new->data;
		int runner =1;
		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				theData [y][x] = atoi(argv[i + runner]);
				runner++;
			}
		}
		*iptr=i+10;	
	return new;
	}
	if(!strcmp(argv[i]+2,"flipHorizontal")){
			new->function = flipHorizontal;
			  *iptr=i+1;
			  return new;
	}
	if(!strcmp(argv[i]+2,"flipVertical")){
		//code goes here
		new->function = flipVertical;
		  *iptr=i+1;
		  return new;
	}		
	return(0);
}	

//define plugin functions
static void rotate(pixMap *p, pixMap *oldPixMap,int y, int x,void *data){
	float *sc=(float*) data;
	 const float ox=p->imageWidth/2.0f;
	 const float oy=p->imageHeight/2.0f;
	 const float s=sc[0];
		const float c=sc[1];
		float rotx = c*(x-ox) - s * (oy-y) + ox;
	 float roty = -(s*(x-ox) + c * (oy-y) - oy);
	 int rotj=rotx+.5;
		int roti=roty+.5;
		if(roti >=0 && roti < oldPixMap->imageHeight && rotj >=0 && rotj < oldPixMap->imageWidth){
	   memcpy(p->pixArray_overlay[y]+x,oldPixMap->pixArray_overlay[roti]+rotj,sizeof(rgba));
			}
			else{
	   memset(p->pixArray_overlay[y]+x,0,sizeof(rgba));
			}
}

//
//      set accumulator to zero
//
//      for each kernel row in kernel:
//         for each element in kernel row:
//
//            if element position  corresponding* to pixel position then
//               multiply element value  corresponding* to pixel value
//               add result to accumulator
//            endif
//
//      set output image pixel to accumulator
//
//
// sum = 0
//for k = -1 to 1 do
//   for j = -1 to 1 do
//      sum = sum + h(j +1, k + 1)*f(x - j, y - k)
//   end for
//end for
//g(x, y) = sum

//Too long but get's the job done : )
static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
	//implement algorithm givne in https://en.wikipedia.org/wiki/Kernel_(image_processing)
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 0;
	//assume that the kernel is a 3x3 matrix of integers
	int (*karnel) [3] = data;
	int accumulator = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			accumulator = accumulator + karnel[i][j];
		}
	}


	for (int kRow = 0; kRow < 3; kRow++) {
		int y = kRow+i -1;
		if (y > oldPixMap->imageHeight-1)
			y = oldPixMap->imageHeight-1;
		if (y < 0)
			y = 0;

		for (int kCol = 0; kCol < 3; kCol++) {
			int x = kCol+j -1;
			if (x < 0)
				x = 0;
			if (y > oldPixMap->imageWidth-1)
				y = oldPixMap->imageWidth-1;
				//might have to cast   --FLAG
			int newR = oldPixMap->pixArray_overlay[y][x].r * (karnel[kRow][kCol]);
			int newG = oldPixMap->pixArray_overlay[y][x].g * (karnel[kRow][kCol]);
			int newB = oldPixMap->pixArray_overlay[y][x].b * (karnel[kRow][kCol]);
			int newA = oldPixMap->pixArray_overlay[y][x].a * (karnel[kRow][kCol]);
			//if the sum = 0 don't normalize;
			if (accumulator == 0) {
				accumulator = 1;
				//fprintf(stderr,"sum is zero");
					if (newR < 0)
						newR = (-newR);
					if (newG < 0)
						newG = (-newG);
					if (newB < 0)
						newB = (-newB);
					if (newA < 0)
						newA = (-newA);
			}
				r = r + newR;
				g = g + newG;
				b = b + newB;
				a = a + newA;
			}
	}
	//don't forget to normalize by dividing by the sum of all the elements in the matrix
	//if sum was 0 this does nothing.
	r = r / accumulator;
	g = g / accumulator;
	b = b / accumulator;
	a = a / accumulator;
	//make sure r g b a is between 255 & 0
	if (r > 255)
		r = 255;
	if (g > 255)
		g = 255;
	if (b > 255)
		b = 255;
	if (a > 255)
		a = 255;
	if (r < 0)
		r = 0;
	if (g < 0)
		g = 0;
	if (b < 0)
		b = 0;
	if (a < 0)
		a = 0;
	//Flag
	p->pixArray_overlay[i][j].r = r ;
	p->pixArray_overlay[i][j].g = g ;
	p->pixArray_overlay[i][j].b = b ;
	p->pixArray_overlay[i][j].a = a ;
}

//very simple functions - does not use the data pointer - good place to start
//Flag
static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
 //reverse the pixels vertically - can be done in one line
	p->pixArray_overlay[oldPixMap->imageHeight-i-1][j] = oldPixMap->pixArray_overlay[i][j];
}
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
 //reverse the pixels horizontally - can be done in one line
	p->pixArray_overlay[i][oldPixMap->imageWidth-j-1] = oldPixMap->pixArray_overlay[i][j];
}

