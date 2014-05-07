#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "machine.h"

#define MAX_CLOSURE 100000000

int nb_closure;

struct closure *mk_closure(struct expr *expr, struct env *env){
  assert(nb_closure<MAX_CLOSURE);
  struct closure *cl = malloc(sizeof(struct closure));
  cl->expr = expr;
  cl->env = env;
  nb_closure++;
  return cl;
}

struct configuration *mk_conf(struct closure *cl){
  struct configuration *conf = malloc(sizeof(struct configuration));
  conf->closure = cl;
  conf->stack=NULL;
  return conf;
}

void print_env(const struct env *env){
  if(env==NULL){printf(" #\n");}
  else{printf(" %s ",env->id);print_env(env->next);}
}

struct env *push_env(char *id, struct closure *cl, struct env *env){
  struct env *e = malloc(sizeof(struct env));
  e->id = id;
  e->closure = cl;
  e->next = env;
  //print_env(e);
  return e;
}

struct env *push_rec_env(char *id, struct expr *expr, struct env *env){
  struct env *e = malloc(sizeof(struct env));
  struct closure *cl = mk_closure(expr,e);
  e-> id = id;
  e->closure = cl;
  e->next = env;
  //print_env(e);
  return e;
}

struct closure *search_env(char *id, struct env *env){
  assert(env!=NULL);
  if(strcmp(id,env->id)==0){return env->closure;}
  else{return search_env(id,env->next);}  
}

struct stack *pop_stack(struct stack *stack){
  struct stack *next = stack->next;
  free(stack);
  return next;
}

struct stack *push_stack(struct closure *cl, struct stack *stack){
  struct stack *st = malloc(sizeof(struct stack));
  st->closure = cl;
  st->next = stack;
  return st;
}

//--------------------------------------------------------------

struct expr* translation(struct configuration *conf,struct expr f,struct cell* vec){
  struct env *env = conf->closure->env; 
  int x;
  int y;
  int xvec;
  int yvec;
  switch(f.type){
  case POINT:

   conf=mk_conf(mk_closure(f.expr->cell.left,env)); 
   step(conf);
   if(conf->closure->expr->type != NUM)
     exit(EXIT_FAILURE);
   x=conf->closure->expr->expr->num;
   conf=mk_conf(mk_closure(f.expr->cell.right,env)); 
   step(conf);
   if(conf->closure->expr->type != NUM)
     exit(EXIT_FAILURE);
   y=conf->closure->expr->expr->num;

   conf=mk_conf(mk_closure(vec->left,env)); 
   step(conf);
   if(conf->closure->expr->type != NUM)
     exit(EXIT_FAILURE);
   xvec=conf->closure->expr->expr->num;
   conf=mk_conf(mk_closure(vec->right,env)); 
   step(conf);
   if(conf->closure->expr->type != NUM)
     exit(EXIT_FAILURE);
   yvec=conf->closure->expr->expr->num;

   return mk_point(mk_int(x+xvec),mk_int(y+yvec));

  case CIRCLE:
    return mk_circle(translation(conf,*f.expr->cell.left,vec),f.expr->cell.right);
  case PATH:
    if(f.expr->cell.right == NULL)
      return mk_path(translation(conf,*f.expr->cell.left,vec),NULL);
    return mk_path(translation(conf,*f.expr->cell.left,vec),translation(conf,*f.expr->cell.right,vec));
   case BEZIER:
    return mk_bezier(translation(conf,*f.expr->bezier.p1,vec),translation(conf,*f.expr->bezier.p2,vec),translation(conf,*f.expr->bezier.p3,vec),translation(conf,*f.expr->bezier.p4,vec));
  case ID:
    conf =mk_conf(mk_closure(&f,env)); 
    step(conf);
    return translation(conf,*conf->closure->expr,vec);
  default: assert(0);
  }
}
struct expr* rotation(struct configuration *conf,struct expr f,struct cell* centre,struct expr angle){
  struct env *env = conf->closure->env; 
  int x;
  int y;
  int xc;
  int yc;
  double ang;

  switch(f.type){
  case POINT:

    conf=mk_conf(mk_closure(f.expr->cell.left,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    x=conf->closure->expr->expr->num;
    conf=mk_conf(mk_closure(f.expr->cell.right,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    y=conf->closure->expr->expr->num;

    conf=mk_conf(mk_closure(centre->left,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    xc=conf->closure->expr->expr->num;
    conf=mk_conf(mk_closure(centre->right,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    yc=conf->closure->expr->expr->num;

    conf=mk_conf(mk_closure(&angle,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    ang= (double)conf->closure->expr->expr->num *M_PI/180;

    // x' = cos(theta)*(x-xc) - sin(theta)*(y-yc) + xc
    // y' = sin(theta)*(x-xc) + cos(theta)*(y-yc) + yc
     
    return mk_point(mk_int (cos(ang)*(x-xc)-sin(ang)*(y-yc)+xc),mk_int(sin(ang)*(x-xc)+cos(ang)*(y-yc)+yc));

  case CIRCLE:
    return mk_circle(rotation(conf,*f.expr->cell.left,centre,angle),f.expr->cell.right);
  case PATH:
    if(f.expr->cell.right == NULL)
      return mk_path(rotation (conf,*f.expr->cell.left,centre,angle),NULL);
    return mk_path(rotation(conf,*f.expr->cell.left,centre,angle),rotation(conf,*f.expr->cell.right,centre,angle));
  case BEZIER:
    return mk_bezier(rotation(conf,*f.expr->bezier.p1,centre,angle),rotation(conf,*f.expr->bezier.p2,centre,angle),rotation(conf,*f.expr->bezier.p3,centre,angle),rotation(conf,*f.expr->bezier.p4,centre,angle));
  case ID:
    conf =mk_conf(mk_closure(&f,env)); 
    step(conf);
    return rotation(conf,*conf->closure->expr,centre,angle);
  default: assert(0);
  }
}

struct expr* homothetie(struct configuration *conf,struct expr f,struct cell* centre,struct expr ratio){
struct env *env = conf->closure->env; 
  int x;
  int y;
  int xc;
  int yc;
  int rat;

  switch(f.type){
  case POINT:

    conf=mk_conf(mk_closure(f.expr->cell.left,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    x=conf->closure->expr->expr->num;
    conf=mk_conf(mk_closure(f.expr->cell.right,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    y=conf->closure->expr->expr->num;

    conf=mk_conf(mk_closure(centre->left,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    xc=conf->closure->expr->expr->num;
    conf=mk_conf(mk_closure(centre->right,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    yc=conf->closure->expr->expr->num;

    conf=mk_conf(mk_closure(&ratio,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    rat= conf->closure->expr->expr->num;

    return mk_point(mk_int(xc+((x-xc)*rat)),mk_int(yc+((y-yc)*rat)));

  case CIRCLE:
    conf=mk_conf(mk_closure(&ratio,env)); 
    step(conf);
    if(conf->closure->expr->type != NUM)
      exit(EXIT_FAILURE);
    rat= conf->closure->expr->expr->num;
    return mk_circle(homothetie(conf,*f.expr->cell.left,centre,ratio),mk_int(f.expr->cell.right->expr->num*rat));
  case PATH:
    if(f.expr->cell.right == NULL)
      return mk_path(homothetie(conf,*f.expr->cell.left,centre,ratio),NULL);
    return mk_path(homothetie(conf,*f.expr->cell.left,centre,ratio),homothetie(conf,*f.expr->cell.right,centre,ratio));
  case BEZIER:
    return mk_bezier(homothetie(conf,*f.expr->bezier.p1,centre,ratio),homothetie(conf,*f.expr->bezier.p2,centre,ratio),homothetie(conf,*f.expr->bezier.p3,centre,ratio),homothetie(conf,*f.expr->bezier.p4,centre,ratio));
  case ID:
    conf =mk_conf(mk_closure(&f,env)); 
    step(conf);
    return homothetie(conf,*conf->closure->expr,centre,ratio);
  default: assert(0);
  }
}




int elemStep(struct configuration *conf,struct cell *cell1,struct cell *cell2){
  printf("debut\n");
  struct env *env = conf->closure->env;
  struct configuration* CONF;
  
  if(cell1 == NULL || cell2 == NULL)
    return 0;

  if(cell1->left==NULL || cell2->left ==NULL){
    if(cell1->left==NULL && cell2->left ==NULL)
      return 1;
    return 0;
  }
  struct cell *tabcell[2]={cell1,cell2};
  int tabres[4]={NIL,0,NIL,0};
  int i;
  for(i=0;i<2;i++){
    CONF= mk_conf(mk_closure(tabcell[i]->left,NULL));
    step(CONF);
    tabres[2*i]=CONF->closure->expr->type;
    if((tabcell[i]->left->type == APP || tabcell[i]->left->type == ID)){
      if(tabres[2*i]== NUM)
	tabres[2*i+1]=CONF->closure->expr->expr->num;
    }else{
      if(tabres[2*i] == NUM)
	tabres[2*i+1]=tabcell[i]->left->expr->num;
    }
  }
  if(tabres[0]==tabres[2]){
    if(tabres[0] == NUM && tabres[1] != tabres[3])
      return 0;
   
    if(cell1->right == NULL && cell2->right == NULL)
      return 1;
    if(cell1->right == NULL || cell2->right == NULL)
      return 0;
     if(tabres[0] == CELL)
      return elemStep(conf,&cell1->left->expr->cell,&cell2->left->expr->cell)  && elemStep(conf,&cell1->right->expr->cell,&cell2->right->expr->cell);
    
    return elemStep(conf,&cell1->right->expr->cell,&cell2->right->expr->cell);
  }
  return 0;
}

void step(struct configuration *conf){
  struct expr *expr = conf->closure->expr;
  struct env *env = conf->closure->env;
  struct stack *stack = conf->stack;
  assert(expr!=NULL);
  switch (expr->type){
  case FUN: 
    {// printf("fun\n");
      if(stack==NULL){return;}
      char *var = expr->expr->fun.id;
      struct expr *body = expr->expr->fun.body;
      struct env *e = push_env(var, stack->closure,env);
      conf->closure=mk_closure(body,e);
      conf->stack = pop_stack(stack);
      return step(conf);
    }
  case APP: 
    { 
      struct expr *fun = expr->expr->app.fun;
      struct expr *arg = expr->expr->app.arg;
      conf->closure = mk_closure(fun,env);
      conf->stack = push_stack(mk_closure(arg,env),conf->stack);
      return step(conf);
    }
  case ID: //printf("id: %s\n", expr->expr->id);
    assert(env!=NULL);
    conf->closure = search_env(expr->expr->id,env);
    return step(conf);
  case COND:
    { 
      struct stack *stack = conf->stack;
      struct closure *cl_cond = mk_closure(expr->expr->cond.cond,env);
      conf->closure = cl_cond;
      conf->stack=NULL;
      step(conf);
      assert(conf->closure->expr->type==NUM);
      conf->stack=stack;
      if(conf->closure->expr->expr->num==0){
        //printf("cond false\n");
        conf->closure = mk_closure(expr->expr->cond.else_br,env);
      }
      else{
        //printf("cond false\n");
        conf->closure = mk_closure(expr->expr->cond.then_br,env);
      }
      return step(conf);
    }
  case NUM: 
    return;
  case CELL:
    conf->closure = mk_closure(expr->expr->cell.left,env);
    return ;
  case NIL:
    return ;
  case POINT:
    return;
  case PATH:
    return;
  case CIRCLE:
    return;
  case BEZIER:
    return ;
  case OP: 
    {
      struct stack *stack = conf->stack;
      /* 
	 dans le cas où la pile de la machine est vide,
	 l'opérateur ne reçoit pas d'argument et le calcul 
	 est terminé. La fonction step s'arrête.
      */
      if(stack==NULL){return;}
      /*
	si en revanche la pile n'est pas vide, il faut alors 
	évaluer la cloture en sommet de pile.
      */
      struct closure *arg1 = stack->closure;
      stack = pop_stack(stack);
      conf->closure = arg1;
      conf->stack = NULL;
      step(conf);
      /*
	Ici lorsque l'on n'avait que des opérations sur les entiers,
	on vérifiait que le résultat était un entier avant de le stocker 
	dans la variable k1. Si le résultat n'était pas un entier, le 
	programme s'arrête parce que le type n'est pas compatible avec 
	l'opération.

	Maintenant que nous avons également des opération sur des listes,
	il vaut mieux différer ce test et la récupération de la valeur
	au moment où l'on connaît l'opétation.
      */
      
      /*
	Auparavant la seule opération unaire possible était NOT, maintenant, 
	on a également TOP et NEXT. Il vaut donc mieux utiliser une structure 
	switch. N'oubliez pas que l'élément de sommet de liste peut être une 
	fonction qui doit s'évaluer avec le pile courante et que le calcul 
	peut ne pas être terminé une fois que l'opération TOP a été effectuée.
      */
      struct expr * k1 = conf->closure->expr;
      if(expr->expr->op==NOT){
	if(conf->closure->expr->type!=NUM){exit(EXIT_FAILURE);}
	k1->expr->num = !k1->expr->num;
	return;
      }

      if(expr->expr->op==TOP){
	if(conf->closure->expr->type!=CELL){exit(EXIT_FAILURE);}
	k1 = k1->expr->cell.left;
	conf->closure = mk_closure(k1,NULL);
	return;
      }

      if(expr->expr->op==NEXT){
	if(conf->closure->expr->type!=CELL){exit(EXIT_FAILURE);}
	k1 = k1->expr->cell.right;
	conf->closure = mk_closure(k1,NULL);
	return;
      }
      /* 
	 Si jamais l'opérateur est unaire et que l'on n'a qu'un seul argument 
	 le calcul est terminé et la fonction step s'arrête
      */
      if(stack==NULL){return;}
      arg1=conf->closure;
      /* 
	 Comme précédemment, si la pile n'est pas vide, on peut évaluer la 
	 cloture de sommet de pile comme précédemment. 
      */
      struct closure *arg2 = stack->closure;
      stack = pop_stack(stack);
      conf->closure = arg2;
      conf->stack=NULL;
      step(conf);
      /*
	Ici encore, il vaut mieux différer le test du type
	et le stockage de la valeur au moment où l'on teste
	la valeur de l'opérateur.
      */
      struct expr *k2 = conf->closure->expr;
      /*
	Ici, on teste la valeur de l'opérateur dans le cas où il est binaire.
	Il faut ici implémenter l'opération CONS. On veut pouvoir construire
	des listes contenant n'importe quel type d'élément. Il n'est donc
	pas utile de tester de type du permier argument  de la CONS, en 
	revanche le second argument de CONS doit être une liste.
      */
      if(expr->expr->op== ROT || expr->expr->op==HOMO){
	struct closure *arg3 = stack->closure;
	stack = pop_stack(stack);
	conf->closure = arg3;
	conf->stack=NULL;
	step(conf);
	struct expr *k3 = conf->closure->expr;
	switch (expr->expr->op){
	case HOMO:
	  conf->closure=mk_closure(homothetie(conf,*k1,&k2->expr->cell,*k3),NULL);
	  return;
	case ROT:
	  conf->closure=mk_closure(rotation(conf,*k1,&k2->expr->cell,*k3),NULL);
	  return;
	default: assert(0);
	    }
      }

	
      switch (expr->expr->op){
      case TRANS:
	conf->closure=mk_closure(translation(conf,*k1,&k2->expr->cell),NULL);
	return ;
      case PLUS: //printf("plus\n");
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);
	}	
	conf->closure = mk_closure(mk_int(k1->expr->num+k2->expr->num),NULL);
	return;
      case MINUS: //printf("minus\n");
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);
	}
	conf->closure = mk_closure(mk_int(k1->expr->num-k2->expr->num),NULL);
	return;
      case MULT: //printf("mult\n");
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);
	}
	conf->closure = mk_closure(mk_int(k1->expr->num*k2->expr->num),NULL);
	return;
      case DIV: 
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);}
	assert(k2->expr->num!=0); 
	conf->closure =  mk_closure(mk_int(k1->expr->num/k2->expr->num),NULL);
	return;
      case LEQ: //printf("%d <= %d \n",k1,k2);
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);}
	conf->closure = mk_closure(mk_int(k1->expr->num <= k2->expr->num),NULL);
	return;
      case LE: 
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);}
	conf->closure = mk_closure(mk_int(k1->expr->num < k2->expr->num),NULL);
	return;
      case GEQ: 
	if((k1->type!=NUM) || (k2->type!=NUM)){
	  exit(EXIT_FAILURE);}
	conf->closure = mk_closure(mk_int(k1->expr->num >= k2->expr->num),NULL); 
	return;
      case GE: 
	if((k2->type!=NUM) || (k1->type!=NUM)){
	  exit(EXIT_FAILURE);}
	conf->closure = mk_closure(mk_int(k1->expr->num > k2->expr->num),NULL); 
	return;
      case EQ:
	if((k1->type==NUM) && (k2->type==NUM)){
	  conf->closure = mk_closure(mk_int(k1->expr->num == k2->expr->num),NULL);
	}else{
	  if((k1->type==CELL ||k1->type==NIL ) && (k2->type==CELL ||k2->type==NIL )){
	    int test= elemStep(conf,&k1->expr->cell,&k2->expr->cell);
	    conf->closure = mk_closure(mk_int(test),NULL);
	}else{
	    exit(EXIT_FAILURE);
	}}
	
	return;
      case OR: if((conf->closure->expr->type!=NUM) || (conf->closure->expr->type!=NUM)){exit(EXIT_FAILURE);}conf->closure = mk_closure(mk_int(k1->expr->num || k2->expr->num),NULL); return;
      case AND:if((conf->closure->expr->type!=NUM) || (conf->closure->expr->type!=NUM)){exit(EXIT_FAILURE);} conf->closure = mk_closure(mk_int(k1->expr->num && k2->expr->num),NULL); return;
      case PUSH: if(conf->closure->expr->type==CELL){conf->closure = mk_closure(mk_cell(k1,k2),NULL);}
	else if(conf->closure->expr->type == NIL){conf->closure = mk_closure(mk_cell(k1,k2->expr->cell.right),NULL);}
	         else{exit(EXIT_FAILURE);}return;
      default: assert(0);
      }   
    }
    ;
  default: assert(0);
  }
}
