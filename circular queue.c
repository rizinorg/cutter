#include<stdio.h>
#include<conio.h>
#include<stdlib.h>
#define SIZE 5
int queue[SIZE],front=-1,rear=-1; 
void inqueue();
void dequeue();
void display();
main(){
	int choice;
	while(1)
	{
		printf("\n Main Menue"); 
		printf("1.\n insert from inqueue");
		printf("2.\n delete from dequeue");
		printf("3.\n display the value");
		printf("4.\n exit");
		printf("\n enter your choice (1-4):");
		printf("\n %d",&choice);
		switch (choice)
		
		{
			    case 1:
				enqueue();
			    	break;
		        case 2:
			      dequeue();
			      break;
		        case 3:
			      display();
				  break;
				  
		        case 4:
			     exit(0); 		  		
		}
	}
	getch();
}

void enqueue()
{
	int n;
	if((front==0 && rear==SIZE-1) || (rear+1==front))
	{
		printf("\n queue is overflow");
		
	}
	else
	{
		printf("\n enter the number");
		scanf("%d",&n);
		if(rear<0)
		{
			front=rear=0;
			queue[rear]=n;
		}
		else if(rear==SIZE-1)
		{
			rear=0;
			queue[rear];
			
		}
		else{
			rear++;
			queue[rear];
		}
	}
		
}


void dequeue()
{
	if(front<0)
	{
		printf("queue is underflow");
		
	}
	else{
		printf("\n deleted value is =%d",queue[front]);
		
		if(front==rear)
		{
			front=rear=-1;
		}
		else if(front==SIZE-1)
		{
			front=0;
		}
		else
		{
			front++;
		}
	}
}


void display()
{
	int i;
	if(front<rear)
	{
		for(i=front;i<=rear;i++)
		{
			printf("%d \t",queue[i]);
		}
	}
	else
	{
		for(i=front;i<SIZE;i++)
		{
			printf("%d",queue[i]);
		}
		for(i=0;i<=rear;i++)
		{
			printf("%d \t",queue[i]);
		}
	}
}// end of display /////
