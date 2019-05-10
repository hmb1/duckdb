
#include "nco_ply_lst.h"


/************************ functions that manipulate lists of polygons ****************************************************/
void
nco_poly_re_org_lst(  /* for each poly_sct*  in list re-order points so that first point is the leftermost point */
poly_sct **pl_lst,
int arr_nbr)
{
  int idx=0;
  int jdx=0;
  int max_crn_nbr=0;

  double *lcl_dp_x;
  double *lcl_dp_y;

  /* max crn_nbr */
  for(idx=0 ; idx<arr_nbr ;idx++)
    if( pl_lst[idx]->crn_nbr > max_crn_nbr )
      max_crn_nbr = pl_lst[idx]->crn_nbr;

  lcl_dp_x=(double*)nco_calloc(max_crn_nbr, sizeof(double));
  lcl_dp_y=(double*)nco_calloc(max_crn_nbr, sizeof(double));


  for(idx=0; idx<arr_nbr; idx++)
  {
    int lcl_min=0;
    int crn_nbr=pl_lst[idx]->crn_nbr;
    double x_min=1.0e-30;

    /* de-reference */
    poly_sct *pl=pl_lst[idx];

    /* find index of min X value */
    for(jdx=0; jdx<crn_nbr; jdx++)
      if( pl->dp_x[jdx] < x_min )
      { x_min=pl->dp_x[jdx]; lcl_min=jdx;}

    /* first point already x_min so do nothing */
    if( lcl_min == 0)
      continue;

    for(jdx=0; jdx<crn_nbr; jdx++)
    {
      lcl_dp_x[jdx]=pl->dp_x[(jdx+lcl_min)%crn_nbr];
      lcl_dp_y[jdx]=pl->dp_y[(jdx+lcl_min)%crn_nbr];
    }



    /* copy over values */
    memcpy(pl->dp_x, lcl_dp_x, (size_t)crn_nbr*sizeof(double));
    memcpy(pl->dp_y, lcl_dp_y, (size_t)crn_nbr*sizeof(double));
  }

  lcl_dp_x=(double*)nco_free(lcl_dp_x);
  lcl_dp_y=(double*)nco_free(lcl_dp_y);

  return;

}






poly_sct **             /* [O] [nbr]  size of array */
nco_poly_lst_mk(
double *area, /* I [sr] Area of source grid */
int *msk, /* I [flg] Mask on source grid */
double *lat_ctr, /* I [dgr] Latitude  centers of source grid */
double *lon_ctr, /* I [dgr] Longitude centers of source grid */
double *lat_crn, /* I [dgr] Latitude  corners of source grid */
double *lon_crn, /* I [dgr] Longitude corners of source grid */
size_t grd_sz, /* I [nbr] Number of elements in single layer of source grid */
long grd_crn_nbr, /* I [nbr] Maximum number of corners in source gridcell */
nco_grd_lon_typ_enm grd_lon_typ, /* I [num] if not nil then split cells that straddle Greenwich or Dateline  */
poly_typ_enm pl_typ,
int *pl_nbr)
{
  const char fnc_nm[]="nco_poly_lst_mk()";

  int idx=0;
  int idx_cnt=0;
  int cnt_wrp_good=0;

  nco_bool bwrp;


  double *lat_ptr=lat_crn;
  double *lon_ptr=lon_crn;

  /* buffers  used in nco-poly_re_org() */
  double lcl_dp_x[VP_MAX]={0};
  double lcl_dp_y[VP_MAX]={0};

  poly_sct *pl;
  poly_sct *pl_wrp_left;
  poly_sct *pl_wrp_right;
  poly_sct **pl_lst;

  /* start with twice the grid size as we may be splitting the cells along the Greenwich meridian or dateline */
  /* realloc at the end */
  pl_lst=(poly_sct**)nco_malloc( sizeof (poly_sct*) * grd_sz  *2 );

  // printf("About to print poly sct   grd_sz=%d grd_crn_nbr=%d\n", grd_sz, grd_crn_nbr);
  for(idx=0;idx<grd_sz; idx++)
  {
    /* check mask and area */
    if( msk[idx]==0 || area[idx] == 0.0)
      continue;


    pl=nco_poly_init_lst(pl_typ, grd_crn_nbr,0, idx, lon_ptr, lat_ptr);
    lon_ptr+=(size_t)grd_crn_nbr;
    lat_ptr+=(size_t)grd_crn_nbr;

    /* if poly is less  than a triangle then  null is returned*/
    if(!pl)
      continue;


    /* add min max */
    nco_poly_minmax_add(pl, grd_lon_typ, False);

    nco_poly_re_org(pl, lcl_dp_x, lcl_dp_y);

    /* use Charlie's formula */
    nco_poly_area_add(pl);


    //if(pl->dp_x_minmax[0] <0.0 || (pl->dp_x_minmax[1] - pl->dp_x_minmax[0]) > 30  )
    if( !(pl->dp_x_minmax[1] - pl->dp_x_minmax[0] < 180.0
             &&  lon_ctr[idx] >= pl->dp_x_minmax[0] && lon_ctr[idx] <= pl->dp_x_minmax[1] ))

    {
      (void)fprintf(stdout, "/***%s: %s: invalid polygon to follow *******?", nco_prg_nm_get(), fnc_nm);
      nco_poly_prn(pl, 0);
      pl=nco_poly_free(pl);
      continue;

    }

    //fprintf(stdout,"/***** input polygon pl lon center=%f   convex=%s\n    ********************/\n", lon_ctr[idx],   (nco_poly_is_convex(pl) ? "True": "False") );
    //nco_poly_prn(pl, 0);



    /* check for wrapping -center outside min/max range */
    bwrp=(  lon_ctr[idx] < pl->dp_x_minmax[0] || lon_ctr[idx] > pl->dp_x_minmax[1] );

    if( grd_lon_typ == nco_grd_lon_nil || grd_lon_typ == nco_grd_lon_unk )
    {



      if( !bwrp  )
      {
        pl_lst[idx_cnt++]=pl;
      }
      else
      {
        (void)fprintf(stdout, "%s:  polygon(%d) wrapped - but grd_lon_typ not specified \n", nco_prg_nm_get(), idx);
        (void)fprintf(stdout, "/*******************************************/\n");

        pl=nco_poly_free(pl);

      }
      continue;
    }


    /* if we are here then grd_lon_typ specifys a grid type  */
    if( !bwrp)
    {
      pl_lst[idx_cnt++]=pl;
    }
      /* cell width exceeds max so assume wrapping */
    else if(  nco_poly_wrp_splt(pl, grd_lon_typ, &pl_wrp_left, &pl_wrp_right ) == NCO_NOERR )
    {

      fprintf(stdout,"/***** pl, wrp_left, wrp_right ********************/\n");


      if(pl_wrp_left)
      {
        nco_poly_re_org(pl_wrp_left, lcl_dp_x, lcl_dp_y);
        pl_lst[idx_cnt++]=pl_wrp_left;
        nco_poly_prn(pl_wrp_left, 2);

      }

      if(pl_wrp_right)
      {
        nco_poly_re_org(pl_wrp_right, lcl_dp_x, lcl_dp_y);
        pl_lst[idx_cnt++]=pl_wrp_right;
        nco_poly_prn(pl_wrp_right, 2);

      }

      pl=nco_poly_free(pl);


      fprintf(stdout,"/**********************************/\n");

      cnt_wrp_good++;
    }
    else
    {
      if(nco_dbg_lvl_get() >=  nco_dbg_std ){
        (void)fprintf(stdout, "%s: split wrapping didnt work on this polygon(%d)\n", nco_prg_nm_get(), idx );
        (void)fprintf(stdout, "/********************************/\n");
      }

      pl=nco_poly_free(pl);
    }


  }

  if(nco_dbg_lvl_get() >=  nco_dbg_std )
    (void)fprintf(stdout, "%s: %s size input list(%lu), size output list(%d), num of split polygons(%d)\n", nco_prg_nm_get(),fnc_nm, grd_sz, idx_cnt, cnt_wrp_good);

  pl_lst=(poly_sct**)nco_realloc( pl_lst, (size_t)idx_cnt * sizeof (poly_sct*) );

  *pl_nbr=idx_cnt;

  return pl_lst;

}


poly_sct **             /* [O] [nbr]  size of array */
nco_poly_lst_mk_sph(
double *area, /* I [sr] Area of source grid */
int *msk, /* I [flg] Mask on source grid */
double *lat_ctr, /* I [dgr] Latitude  centers of source grid */
double *lon_ctr, /* I [dgr] Longitude centers of source grid */
double *lat_crn, /* I [dgr] Latitude  corners of source grid */
double *lon_crn, /* I [dgr] Longitude corners of source grid */
size_t grd_sz, /* I [nbr] Number of elements in single layer of source grid */
long grd_crn_nbr, /* I [nbr] Maximum number of corners in source gridcell */
nco_grd_lon_typ_enm grd_lon_typ, /* I [num]  */
poly_typ_enm pl_typ,
int *pl_nbr)
{

  int idx=0;
  int idx_cnt=0;
  int wrp_cnt=0;
  int wrp_y_cnt=0;

  const char fnc_nm[]="nco_poly_lst_mk()";

  nco_bool bwrp;
  double tot_area=0.0;

  double *lat_ptr=lat_crn;
  double *lon_ptr=lon_crn;

  /* buffers  used in nco-poly_re_org() */
  double lcl_dp_x[VP_MAX]={0};
  double lcl_dp_y[VP_MAX]={0};

  poly_sct *pl=(poly_sct*)NULL_CEWI;

  poly_sct **pl_lst;

  /* start with twice the grid size as we may be splitting the cells along the Greenwich meridian or dateline */
  /* realloc at the end */
  pl_lst=(poly_sct**)nco_malloc( sizeof (poly_sct*) * grd_sz  *2 );


  /* filter out wrapped lon cells */
  if( grd_lon_typ == nco_grd_lon_nil || grd_lon_typ == nco_grd_lon_unk || grd_lon_typ == nco_grd_lon_bb )
   bwrp=False;
  else
    bwrp=True;

  // printf("About to print poly sct   grd_sz=%d grd_crn_nbr=%d\n", grd_sz, grd_crn_nbr);
  for(idx=0;idx<grd_sz; idx++)
  {
    /* check mask and area */
    if( msk[idx]==0 || area[idx] == 0.0 )
      continue;

    pl=nco_poly_init_lst(pl_typ, grd_crn_nbr,0, idx, lon_ptr, lat_ptr);
    lon_ptr+=(size_t)grd_crn_nbr;
    lat_ptr+=(size_t)grd_crn_nbr;

    /* if poly is less  than a triangle then  null is returned*/
    if(!pl) {

      if(nco_dbg_lvl_get()>= nco_dbg_dev)
         fprintf(stderr, "%s(): WARNING cell(id=%d) less than a triange\n", fnc_nm, idx);

      continue;
    }
    /* add centroid from input  */
    pl->dp_x_ctr=lon_ctr[idx];
    pl->dp_y_ctr=lat_ctr[idx];


    /* pop shp */
    nco_poly_shp_pop(pl);

    /* add min max */
    nco_poly_minmax_add(pl, grd_lon_typ, True);

    /* manually add wrap flag */
    // pl->bwrp= (fabs(pl->dp_x_minmax[1] - pl->dp_x_minmax[0]) >= 180.0);

    /* if coords cannot deal with wrapping */
    if( pl->bwrp  && bwrp==False   )
    {
      pl=nco_poly_free(pl);
      continue;
    }

    /* make leftermost vertex first in array */

    nco_poly_re_org(pl, lcl_dp_x, lcl_dp_y);

    pl->area=area[idx];

    /* use Charlie's formula */
    nco_poly_area_add(pl);




    /* add centers
    nco_poly_ctr_add(pl, grd_lon_typ);
    if(pl->bwrp)
      (void)fprintf(stderr,"%s:%s(): comp_center  pl(%f,%f) in(%f, %f)\n", nco_prg_nm_get(),  fnc_nm, pl->dp_x_ctr, pl->dp_y_ctr, lon_ctr[idx], lat_ctr[idx] );
    */

    if(nco_dbg_lvl_get()>= nco_dbg_dev  )
      if(pl->bwrp)
        nco_poly_prn(pl,0);

    /* for debugging */
    tot_area+=pl->area;

    /* for debugging total number of wrapped cells */
    wrp_cnt+=pl->bwrp;
    wrp_y_cnt+=pl->bwrp_y;

    pl_lst[idx_cnt]=pl;
    idx_cnt++;


  }

  if(nco_dbg_lvl_get() >=  nco_dbg_dev )
    (void)fprintf(stderr, "%s: %s size input list(%lu), size output list(%d)  total area=%.15e  num wrapped= %d num caps=%d\n", nco_prg_nm_get(),fnc_nm, grd_sz, idx_cnt, tot_area, wrp_cnt, wrp_y_cnt);

  pl_lst=(poly_sct**)nco_realloc( pl_lst, (size_t)idx_cnt * sizeof (poly_sct*) );

  *pl_nbr=idx_cnt;

  return pl_lst;

}

poly_sct **
nco_poly_lst_free(
poly_sct **pl_lst,
int arr_nbr)
{
  int idx;

  for(idx=0; idx<arr_nbr; idx++)
    pl_lst[idx]=nco_poly_free(pl_lst[idx]);

  pl_lst=(poly_sct**)nco_free(pl_lst);

  return pl_lst;

}


void
nco_poly_set_priority(
int nbr_lst,
KDPriority *list){

  int idx;

  for(idx=0;idx<nbr_lst;idx++){

    list[idx].dist = 1.1;
    list[idx].elem = (KDElem*)NULL;
  }

  return ;

}

poly_sct **
nco_poly_lst_mk_vrl(   /* create overlap mesh  for crt polygons */
poly_sct **pl_lst_in,
int pl_cnt_in,
poly_sct **pl_lst_out,
int pl_cnt_out,
int *pl_cnt_vrl_ret){

/* just duplicate output list to overlap */

  size_t idx;
  size_t jdx;

  int max_nbr_vrl=1000;
  int pl_cnt_vrl=0;

  nco_bool bSort=True;

  const char fnc_nm[]="nco_poly_mk_vrl()";

  /* buffers  used in nco-poly_re_org() */
  double lcl_dp_x[VP_MAX]={0};
  double lcl_dp_y[VP_MAX]={0};


  kd_box size;

  poly_sct ** pl_lst_vrl=NULL_CEWI;

  KDElem *my_elem;
  KDTree *rtree;

  KDPriority *list;

  list = (KDPriority *)nco_calloc(sizeof(KDPriority),(size_t)max_nbr_vrl);

  printf("INFO - entered function nco_poly_mk_vrl\n");

  /* create kd_tree from output polygons */
  rtree=kd_create();

  /* populate kd_tree */
  for(idx=0 ; idx<pl_cnt_out;idx++){


    my_elem=(KDElem*)nco_calloc((size_t)1,sizeof (KDElem) );

    size[KD_LEFT]  =  pl_lst_out[idx]->dp_x_minmax[0];
    size[KD_RIGHT] =  pl_lst_out[idx]->dp_x_minmax[1];

    size[KD_BOTTOM] = pl_lst_out[idx]->dp_y_minmax[0];
    size[KD_TOP]    = pl_lst_out[idx]->dp_y_minmax[1];

    //chr_ptr=(char*)pl_lst_out[idx];

    kd_insert(rtree, (kd_generic)pl_lst_out[idx], size, (char*)my_elem);

  }

  /* rebuild tree for faster access */
  kd_rebuild(rtree);
  kd_rebuild(rtree);

  /* kd_print(rtree); */

/* start main loop over input polygons */
  for(idx=0 ; idx<pl_cnt_in ;idx++ )
  {
    int cnt_vrl=0;
    int cnt_vrl_on=0;

    (void)nco_poly_set_priority(max_nbr_vrl,list);
    /* get bounds of polygon in */
    size[KD_LEFT]  =  pl_lst_in[idx]->dp_x_minmax[0];
    size[KD_RIGHT] =  pl_lst_in[idx]->dp_x_minmax[1];

    size[KD_BOTTOM] = pl_lst_in[idx]->dp_y_minmax[0];
    size[KD_TOP]    = pl_lst_in[idx]->dp_y_minmax[1];

    /* find overlapping polygons */

    cnt_vrl=kd_nearest_intersect(rtree, size, max_nbr_vrl,list,bSort );


    /* nco_poly_prn(2, pl_lst_in[idx] ); */


    for(jdx=0; jdx <cnt_vrl ;jdx++){

      poly_sct *pl_vrl=(poly_sct*)NULL_CEWI;
      poly_sct *pl_out=(poly_sct*)list[jdx].elem->item;           ;


      // nco_poly_prn(2, pl_out);

      /* check for polygon in polygon first */
      if( nco_poly_poly_in_poly(pl_lst_in[idx], pl_out) == pl_out->crn_nbr )
      {
        //fprintf(stderr,"%s: using poly_in_poly()\n", fnc_nm);
        pl_vrl=nco_poly_dpl(pl_out);
      }
      else
        pl_vrl= nco_poly_vrl_do(pl_lst_in[idx], pl_out);

      if(pl_vrl){
        nco_poly_re_org(pl_vrl, lcl_dp_x, lcl_dp_y);
        /* add area */
        nco_poly_area_add(pl_vrl);
        /* shp not needed */
        nco_poly_shp_free(pl_vrl);

        pl_lst_vrl=(poly_sct**)nco_realloc(pl_lst_vrl, sizeof(poly_sct*) * (pl_cnt_vrl+1));
        pl_lst_vrl[pl_cnt_vrl]=pl_vrl;
        pl_cnt_vrl++;
        cnt_vrl_on++;

        if(nco_poly_is_convex(pl_vrl) == False )
        {
          fprintf(stderr,"%s: %s vrl polygon convex=0  vrl ,in convex=%d ,out convex=%d\n", nco_prg_nm_get(), fnc_nm, nco_poly_is_convex(pl_lst_in[idx]), nco_poly_is_convex(pl_out) );
          nco_poly_prn(pl_vrl, 2);
          nco_poly_prn(pl_lst_in[idx], 2);
          nco_poly_prn(pl_out, 2);

        }

        //fprintf(stderr,"Overlap polygon to follow\n");
        //nco_poly_prn(2, pl_vrl);

      }


    }

    if( nco_dbg_lvl_get() >= nco_dbg_dev )
      (void) fprintf(stderr, "%s: total overlaps=%d for polygon %lu - potential overlaps=%d actual overlaps=%d\n", nco_prg_nm_get(), pl_cnt_vrl,  idx, cnt_vrl, cnt_vrl_on);


  }


  kd_destroy(rtree,NULL);

  list = (KDPriority *)nco_free(list);

  /* return size of list */
  *pl_cnt_vrl_ret=pl_cnt_vrl;


  return pl_lst_vrl;

}

poly_sct **
nco_poly_lst_mk_vrl_sph(  /* create overlap mesh  for sph polygons */
poly_sct **pl_lst_in,
int pl_cnt_in,
poly_sct **pl_lst_out,
int pl_cnt_out,
nco_grd_lon_typ_enm grd_lon_typ,
int *pl_cnt_vrl_ret){


/* just duplicate output list to overlap */
  nco_bool bDirtyRats=False;
  nco_bool bSplit=False;
  nco_bool bSort=True;
  
  int max_nbr_vrl=1000;
  int pl_cnt_vrl=0;
  int wrp_cnt=0;
  int pl_cnt_dbg=0;

  size_t idx;
  size_t jdx;

  /* used in realloc */
  size_t nbr_vrl_blocks=0;


  const char fnc_nm[]="nco_poly_mk_vrl_sph()";

  /* buffers  used in nco-poly_re_org()
  double lcl_dp_x[VP_MAX]={0};
  double lcl_dp_y[VP_MAX]={0};
  */

  double tot_area=0.0;

  kd_box size1;
  kd_box size2;

  poly_sct ** pl_lst_vrl=NULL_CEWI;
  poly_sct **pl_lst_dbg=NULL_CEWI;

  KDElem *my_elem1;
  KDElem *my_elem2;
  KDTree *rtree;

  KDPriority *list;

  list = (KDPriority *)nco_calloc(sizeof(KDPriority),(size_t)max_nbr_vrl);

  /* create kd_tree from output polygons */
  rtree=kd_create();

  /* populate kd_tree */
  for(idx=0 ; idx<pl_cnt_out;idx++){

    double df=pl_lst_out[idx]->dp_x_minmax[1] - pl_lst_out[idx]->dp_x_minmax[0];

    my_elem1=(KDElem*)nco_calloc((size_t)1,sizeof (KDElem) );

    /* kd tree cannot handle wrapped coordinates so split minmax if needed*/
    bSplit=nco_poly_minmax_split(pl_lst_out[idx], grd_lon_typ, size1, size2 );

    kd_insert(rtree, (kd_generic)pl_lst_out[idx], size1, (char*)my_elem1);

    if(bSplit){
      my_elem2=(KDElem*)nco_calloc((size_t)1,sizeof (KDElem) );
      kd_insert(rtree, (kd_generic)pl_lst_out[idx], size2, (char*)my_elem2);
    }

  }

  /*  2019-04-24  rebuild NOT_WORKING correctly with repsect to extents */
  /* rebuild tree for faster access
  kd_rebuild(rtree);
  kd_rebuild(rtree);
 */

 /* kd_print(rtree); */

/* start main loop over input polygons */
  for(idx=0 ; idx<pl_cnt_in ;idx++ ) {


    int cnt_vrl = 0;
    int cnt_vrl_on = 0;

    double vrl_area = 0.0;

    //double df=pl_lst_in[idx]->dp_x_minmax[1] - pl_lst_in[idx]->dp_x_minmax[0];

    (void) nco_poly_set_priority(max_nbr_vrl, list);

    /* get bounds of polygon in */
    bSplit=nco_poly_minmax_split(pl_lst_in[idx],grd_lon_typ, size1,size2 );


    /* if a wrapped polygon then do two searches */
    if(bSplit)
      cnt_vrl = kd_nearest_intersect_wrp(rtree, size1, size2, list, max_nbr_vrl);
    else
      cnt_vrl = kd_nearest_intersect(rtree, size1, max_nbr_vrl, list, bSort);

    /* nco_poly_prn(2, pl_lst_in[idx] ); */


    for (jdx = 0; jdx < cnt_vrl; jdx++) {

      poly_sct *pl_vrl = NULL_CEWI;
      poly_sct *pl_out = (poly_sct *) list[jdx].elem->item;;

      if (pl_lst_in[idx]->pl_typ != pl_out->pl_typ) {
        fprintf(stderr, "%s: %s poly type mismatch\n", nco_prg_nm_get(), fnc_nm);
        continue;
      }



      pl_vrl = nco_poly_vrl_do(pl_lst_in[idx], pl_out);

      /* if pl_vrl is NULL from,  nco_poly_do_vrl()  then there are 3 possible senario's
       *
       *  1) pl_lst_in[idx] and pl_out are seperate and  distinct
       *
       *  2) pl_lst_in[idx] is entirely inside pl_out.
       *     to check for this it is sufficent to check the
       *     the minmax bounds and then also  check that a single vertex
       *     from pl_lst_in[idx] is inside pl_out
       *
       *  3) pl_out is entirely inside pl_lst_in[idx]
       *     check minmax bounds then check a single vertex from
       *     pl_out
       */

      if (pl_vrl == (poly_sct *)NULL_CEWI) {

        double pControl[NBR_SPH];

        /* see if pl_out completly inside pl_lst_in[idx] */
        if (nco_poly_in_poly_minmax(pl_lst_in[idx], pl_out) )
        {
          if(nco_sph_mk_control( pl_lst_in[idx], pControl  ) &&
            nco_sph_pnt_in_poly(pl_lst_in[idx]->shp, pl_lst_in[idx]->crn_nbr,pControl, pl_out->shp[0] ) )
              pl_vrl = nco_poly_dpl(pl_out);
        }
          /* see if  pl_lst_in[idx] completly inside pl_out   */
        else if ( nco_poly_in_poly_minmax(pl_out, pl_lst_in[idx]))
        {
          if(nco_sph_mk_control( pl_out, pControl  ) &&
            nco_sph_pnt_in_poly(pl_out->shp, pl_out->crn_nbr,pControl, pl_lst_in[idx]->shp[0] ) )
              pl_vrl = nco_poly_dpl(pl_lst_in[idx]);
        }


        /* add aprropriate id's */
        if(pl_vrl)
        {
          pl_vrl->src_id = pl_lst_in[idx]->src_id;
          pl_vrl->dst_id = pl_out->src_id;
        }



      }


      if (pl_vrl) {
        // nco_poly_re_org(pl_vrl, lcl_dp_x, lcl_dp_y);

        /* add area */
        nco_poly_area_add(pl_vrl);

        /* shp not needed */
        nco_poly_shp_free(pl_vrl);

        /* calculate weight -simple ratio of areas */
        pl_vrl->wgt=pl_vrl->area / pl_out->area;

        nco_poly_minmax_add(pl_vrl, grd_lon_typ, False);
        /* manually add wrap */
        /*
        if(pl_vrl->dp_x_minmax[1] - pl_vrl->dp_x_minmax[0] >=180.0 )
          pl_vrl->bwrp=True;
        else
          pl_vrl->bwrp=False;
        */

        /* add lat/lon centers */
        nco_poly_ctr_add(pl_vrl, grd_lon_typ);

        wrp_cnt+=pl_vrl->bwrp;

        vrl_area += pl_vrl->area;

        if( nbr_vrl_blocks * NCO_VRL_BLOCKSIZE  <  pl_cnt_vrl+1  )
          pl_lst_vrl = (poly_sct **) nco_realloc(pl_lst_vrl, sizeof(poly_sct *) * ++nbr_vrl_blocks * NCO_VRL_BLOCKSIZE );



        pl_lst_vrl[pl_cnt_vrl] = pl_vrl;
        pl_cnt_vrl++;
        cnt_vrl_on++;



      }


    } /* end jdx */

    /* charlies area function sometimes returns Nan */
    if( !isnan(vrl_area) )
       tot_area+=vrl_area;


    if (nco_dbg_lvl_get() >= nco_dbg_dev) {
      /* area diff by more than 10% */
      double frc = vrl_area / pl_lst_in[idx]->area;
      if ( frc <0.95 || frc >1.05 ) {
        (void) fprintf(stderr,
                       "%s: polygon %lu - potential overlaps=%d actual overlaps=%d area_in=%.10e vrl_area=%.10e\n",
                       nco_prg_nm_get(), idx, cnt_vrl, cnt_vrl_on, pl_lst_in[idx]->area, vrl_area);



        if (bDirtyRats ) {
          //if (pl_lst_in[idx]->bwrp ) {
          pl_lst_dbg = (poly_sct **) nco_realloc(pl_lst_dbg, sizeof(poly_sct *) * (pl_cnt_dbg + 1));
          pl_lst_dbg[pl_cnt_dbg] = nco_poly_dpl(pl_lst_in[idx]);
          pl_cnt_dbg++;

          if (1) {
            (void) fprintf(stderr, "/** following pl_lst_in[%lu]  **/\n", idx);
            nco_poly_prn(pl_lst_in[idx], 1);
            (void) fprintf(stderr, "/** potential overlaps to  follow  **/\n");
            for (jdx = 0; jdx < cnt_vrl; jdx++)
              nco_poly_prn((poly_sct *) list[jdx].elem->item, 1);

            (void) fprintf(stderr, "/************* end dirty rats ***************/\n");
          }


        }

      }

    }
  }

  /* turn tot_area into a % of 4*PI */
  tot_area = tot_area / 4.0 / M_PI *100.0;

  /* final report */
  if (nco_dbg_lvl_get() >= nco_dbg_dev)
      (void) fprintf(stderr, "%s: total overlaps=%d, total_area(sphere)=%3.10f total num wrapped=%d\n", nco_prg_nm_get(), pl_cnt_vrl, tot_area  , wrp_cnt);


  kd_destroy(rtree,NULL);

  list = (KDPriority *)nco_free(list);


  /* write filtered polygons to file */
  if(bDirtyRats && pl_cnt_dbg)
  {
    nco_msh_poly_lst_wrt("tst-wrt-dbg.nc", pl_lst_dbg, pl_cnt_dbg, grd_lon_typ);

    pl_lst_dbg=(poly_sct**)nco_poly_lst_free(pl_lst_dbg, pl_cnt_dbg);
  }

  /* reduce size */
  if( nbr_vrl_blocks * NCO_VRL_BLOCKSIZE > pl_cnt_vrl  )
    pl_lst_vrl = (poly_sct **) nco_realloc(pl_lst_vrl, sizeof(poly_sct *) * pl_cnt_vrl);



  /* return size of list */
  *pl_cnt_vrl_ret=pl_cnt_vrl;



  return pl_lst_vrl;

}

poly_sct ** 
nco_poly_lst_chk_dbg(
poly_sct **pl_lst,
int pl_cnt,
poly_sct **pl_lst_vrl,
int pl_cnt_vrl,
int io_flg,  /* [flg] 0 - use src_id from vrl, 1 - use dst_id from vrl */
int *pl_cnt_dbg) /* size of output dbg grid */
{
  int id;
  int idx;
  int jdx;

  int pl_nbr_dbg=0;
  double epsilon=1.0e-12;
  double *area=NULL_CEWI;

  nco_bool is_lst_cnt=False;

  /* if true then pl_cnt matches max src_id There are no missing records from NetCDF SCRIP input */
  is_lst_cnt=( pl_cnt== pl_lst[pl_cnt-1]->src_id +1);

  poly_sct **pl_lst_dbg=NULL_CEWI;

  const char fnc_nm[]="nco_poly_lst_chk_dbg()";

  area=(double*)nco_malloc(sizeof(double)*pl_cnt);
  for(idx=0;idx<pl_cnt;idx++)
    area[idx]=pl_lst[idx]->area;


  for(idx=0;idx<pl_cnt_vrl;idx++)
  {

      id = (io_flg ? pl_lst_vrl[idx]->dst_id : pl_lst_vrl[idx]->src_id);

      if(is_lst_cnt )
        area[id] -= pl_lst_vrl[idx]->area;
      else
      {
      for (jdx = 0; jdx < pl_cnt; jdx++)
        if (pl_lst[jdx]->src_id == id)
          break;

      if (jdx < pl_cnt)
        area[jdx] -= pl_lst_vrl[idx]->area;
     }

  }


  for(idx=0;idx<pl_cnt;idx++) {
    if (fabs(area[idx]) > epsilon) {

      if (nco_dbg_lvl_get() >= nco_dbg_dev)
        fprintf(stderr, "%s() src_id=%d area=%.10f\n", fnc_nm, pl_lst[idx]->src_id, area[idx]);

      pl_lst_dbg = (poly_sct **) nco_realloc(pl_lst_dbg, sizeof(poly_sct*) * (pl_nbr_dbg + 1));
      pl_lst_dbg[pl_nbr_dbg] = nco_poly_dpl(pl_lst[idx]);
      pl_nbr_dbg++;
    }
  }


  *pl_cnt_dbg=pl_nbr_dbg;

  return pl_lst_dbg;


}



/* check areas - nb WARNING modifies area in pl_lst_in and pl_lst_out */
void nco_poly_lst_chk(
poly_sct **pl_lst_in,
int pl_cnt_in,
poly_sct **pl_lst_out,
int pl_cnt_out,
poly_sct **pl_lst_vrl,
int pl_cnt_vrl)
{
  int id;
  int idx;
  int jdx;

  double sum=0.0;
  double epsilon=1.0e-8;

  const char fnc_nm[]="nco_poly_lst_chk()";

  for(idx=0;idx<pl_cnt_vrl;idx++)
  {
    id=pl_lst_vrl[idx]->src_id;
    for(jdx=0;jdx<pl_cnt_in;jdx++)
      if(pl_lst_in[jdx]->src_id==id)
        break;

    if(jdx < pl_cnt_in )
      pl_lst_in[jdx]->area-=pl_lst_vrl[idx]->area;

  }

  fprintf(stderr, "%s():WARNING following is list of incomplete src cells, by src_id no\n",fnc_nm);
  for(idx=0;idx<pl_cnt_in;idx++)
    if( fabs(  pl_lst_in[idx]->area) > epsilon)
      fprintf(stderr, "src_id=%d area=%.10f\n", pl_lst_in[idx]->src_id, pl_lst_in[idx]->area );


  for(idx=0;idx<pl_cnt_vrl;idx++)
  {
    id=pl_lst_vrl[idx]->dst_id;
    for(jdx=0;jdx<pl_cnt_out;jdx++)
      if(pl_lst_out[jdx]->src_id==id)
        break;

    if(jdx < pl_cnt_out )
      pl_lst_out[jdx]->area-=pl_lst_vrl[idx]->area;

  }

  fprintf(stderr, "%s():WARNING following is list of incomplete dst cells, by src_id no\n",fnc_nm);
  for(idx=0;idx<pl_cnt_out;idx++)
    if( fabs(  pl_lst_out[idx]->area) > epsilon)
      fprintf(stderr, "src_id=%d area=%.10f\n", pl_lst_out[idx]->src_id, pl_lst_out[idx]->area );





   return;
}