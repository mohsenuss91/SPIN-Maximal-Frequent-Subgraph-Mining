#include "trips.h"

#define SUPINIT 500
#define SUPREALLOC 100
#define PATINIT 5
#define PATREALLOC 5 

void readTrees ( char *file );
void mineTrees ( Pattern &pat, LABEL lab_to_add, int posindex, int extsupport, 
                 Embedding *tidlist, int tidlistsize, int supdsloc, int& totalcnt, int level, LABEL  *freqind, int numfreqind );
void quicksort(int x[], int first, int last) ;
int partition(int y[], int f, int l) ;
int binarySearch(LABEL *a, LABEL value, int left, int right) ;

int minsup;
FILE *patout;
long int spaces = 0;
COUNT *labind;
long *flags;
int MAXLAB;
int F1;

/* -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ */

int main ( int argc, char **argv) {

    TimeTracker tt;
    tt.Start();

    if ( argc != 5 ) {
        fprintf(stderr, "USAGE: a.out <datafile> <support> <maxlab> <outfile> \n");
        exit(1);
    }    

    patout = fopen ( argv[4], "w");
    if ( !patout ) {
        fprintf(stderr, "Can not open the file: %s\n", argv[4]);
        exit(1);
    }

    membuf = Buffer(BUFSIZE);
    membuf.mark_buf(0);

    MAXLAB = atoi( argv[3] ) + 1;
    minsup = atoi( argv[2] );
    labind = (COUNT *) malloc ( sizeof(COUNT) * MAXLAB );
    for ( int i=0; i < MAXLAB; i++ ) {
        labind[i] = 0;
    }

    TimeTracker rt;
    rt.Start();

    readTrees ( argv[1] );
    F1 = MAXLAB;

    fprintf(stderr, "Time taken for Reading: %lf\n", rt.Stop());
    int subtreecnt = 0;

    for (int tid=0; tid < Tree::dbsize; tid++) {
        treeset[tid].patmatchlen = PATINIT * treeset[tid].nodes;
        treeset[tid].patmatch = (struct Match *) malloc ( sizeof(struct Match) * treeset[tid].patmatchlen );
    }

    supds = (SupportStructure *) malloc ( sizeof(SupportStructure) * SUPINIT );
    supsize = SUPINIT;

    TimeTracker ttt;
    ttt.Start();
    
    /* Mine the equivalence classes */
    for ( int labid=0; labid < labcount; labid++ ) {
      if ( labind[labid] >= minsup ) {
        Pattern pat;
        int totalcnt = 0;
        rt.Start();
        
        mineTrees ( pat, labid, -1, 0, NULL, 0, 0, totalcnt, 1, NULL, 0 );
        
        subtreecnt += totalcnt;
      }
    }

    fprintf(stderr, "Total\t%d\t%d\t%lf\t%lf\n", minsup, subtreecnt, tt.Stop(), ttt.Stop());

    free(supds);
    membuf.clear_to_mark(0);
    membuf.clearbuf();
    fclose(patout);

} // main

/* -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ */


void mineTrees ( Pattern &pat, LABEL lab_to_add, int posindex, int extsupport, Embedding *tidlist, 
                 int tidlistsize, int supdsloc, int& totalcnt, int level, LABEL  *freqind, int numfreqind ) {
                
int i, j, ij, tid, ind, newtid_index, start, match_loc, newdsloc, tind, k, patlength, tmploc, index;
bool found, extfound;
PON parent, parent_ind;
LABEL lab, lab_ind;
Embedding *newtidlist;         // newtidlist gives the list of trees and starting locations of new pattern (pat + lab_to_add) 
PON tmpembed[100];
Pattern newpat;

    /*  1. Locate the new pattern (pat + lab_to_add) in each tree 
        2. Create the pointers to pattern from nodes with label lab_to_add.
        3. Create the newtidlist 
    */

    membuf.mark_buf ( level );


    if ( tidlist != NULL ) 
        newtidlist = (Embedding *) membuf.newbuf ( sizeof(Embedding) * extsupport );
    else 
        newtidlist = (Embedding *) membuf.newbuf ( sizeof(Embedding) * Tree::dbsize );
    newtid_index = 0;

    if(newtidlist == NULL) {
        fprintf(stderr, "*** Can not allocate more memory: %d\n", lab_to_add);
        exit(1);
    }

    patlength = pat.nodes;
    newpat.extend ( pat, lab_to_add, posindex );
    
    /* print the frequent pattern */
    if ( extsupport >= minsup ) { 
      //  newpat.print(patout);
        totalcnt++;
    }

    if ( tidlist == NULL ) {
        bool justfound = false;
        
        newdsloc = supdsloc;
        for ( tid=0; tid < Tree::dbsize; tid++ ) {
            Tree *curr = treeset + tid;    // &treeset[tid]
            found = false;

            for ( int ii=supdsloc; ii < newdsloc; ii++ ) 
                supds[ii].flag = 0;

            for ( ind=curr->nodes; ind > 0; ind-- ) {
                
                bool justfound = false;  // labseq[ind] matched lab_to_ind and posindex
                if ( treeset[tid].nodelist[ind].labseq == lab_to_add ) {
                        /* if it is the first match, the update newtidlist. 
                           newtid_index <= #of entries in newtidlist.
                              start loc is always 1 less than the first match loc
                              match loc points to first available position 
                        */
                    if ( found == false ) {
                        found = true;
                        newtidlist [ newtid_index ].tid = tid;
                        newtidlist [ newtid_index ].start_loc = ind - 1;
                        newtid_index++;
                        match_loc = 0;
                    }
                    if ( match_loc >= treeset[tid].patmatchlen ) {
                        treeset[tid].patmatchlen += (PATREALLOC*treeset[tid].nodes);
                        treeset[tid].patmatch = (struct Match *) realloc ( treeset[tid].patmatch, sizeof(struct Match) * treeset[tid].patmatchlen);
                    }
                    treeset[tid].patmatch[ match_loc ].matchpon = ind;
                    treeset[tid].patmatch[ match_loc ].ptr = -1;   // ptr from initial guys (parent of eq. class)
                    justfound = true;
                    match_loc++;
                }

                if ( found && labind[treeset[tid].nodelist[ind].labseq] >= minsup) {
//                if ( found ) { 
                    int ii, pos;

                    if ( justfound ) 
                        ii = match_loc - 2;
                    else
                        ii = match_loc - 1;
                
                    for ( i=0; i <= ii; i++ ) {   // skip the last 0 in patmatch
                        parent = treeset[tid].nodelist[ind].nps;
                        lab = treeset[tid].nodelist[ind].labseq;
                        bool matchfound = false;
                        
                        while (1) { 
                        /* Check if parent is the valid growth poing for lab */
                            if ( treeset[tid].patmatch[i].matchpon == parent ) {
                                bool extfound = false;
                                for ( j=supdsloc; j < newdsloc; j++ ) 
                                    if ( supds[j].lab == lab && supds[j].position == 1 ) {
                                        if( supds[j].flag == 0 ) {
                                            supds[j].sup++;
                                            supds[j].flag = 1;
                                        }
                                        extfound = true;
                                        break;             // if we remove break then, we can count WEIGTHED supp. 
                                    }
                                if ( extfound == false ) {
                                    if ( newdsloc + 1 > supsize ) { 
                                        supsize += SUPREALLOC;
                                        supds = (SupportStructure *) realloc ( supds, sizeof(SupportStructure) * supsize );

                                    }
                                    supds[newdsloc].lab = lab;
                                    supds[newdsloc].position = 1;
                                    supds[newdsloc].sup = 1;
                                    supds[newdsloc].flag = 1;
                                    newdsloc++;
                                }
                                matchfound = true;
                            } 
                        
                            if ( matchfound == false) {
                                parent = treeset[tid].nodelist[parent].nps;
                                if ( parent == 0 || parent > treeset[tid].patmatch[0].matchpon)
                                    break;
                            }
                            else
                                break;
                        } // i -- while
                
                    } // while (1) -- for
                
                } // if (found)
            
            } // ind
            if ( found == true ) {
                if ( match_loc >= treeset[tid].patmatchlen ) {
                    treeset[tid].patmatchlen += (PATREALLOC*treeset[tid].nodes);
                    treeset[tid].patmatch = (struct Match *) realloc ( treeset[tid].patmatch, sizeof(struct Match) * treeset[tid].patmatchlen);
                }
                treeset[tid].patmatch [ match_loc ].matchpon = 0;
                treeset[tid].patmatch [ match_loc ].ptr = -2;  // Separator
                match_loc++;
                newtidlist[newtid_index-1].match_loc = match_loc;
            }
        } // for each tid
        
        if ( newtid_index >= minsup ) {
            //  newpat.print(patout);
            fcounts[newpat.nodes]++;
            totalcnt++;
        }
        else {
            return;
        }

        LABEL *candlist = (LABEL *) membuf.newbuf ( sizeof(LABEL) * (newdsloc-supdsloc+1) );
        // Assuming 5000 th recursion never occurs
        membuf.mark_buf ( 5000 ); // -- easy to fix
        
        // true if it is candidate frequent item ..
        bool *labflag = (bool *) membuf.newbuf ( sizeof(bool) * (F1+1));
        for ( int ii=0; ii <= F1; ii++)
            labflag[ii] = false;
            
        int cnt =0;
        for ( i=supdsloc; i < newdsloc; i++ ) {
            if ( supds[i].sup >= minsup ) {
                if ( labflag[ supds[i].lab ] == false ) {
                    labflag[ supds[i].lab ] = true;
                    candlist[cnt] = supds[i].lab;
                    cnt++;
                }
            }
        }

        membuf.clear_to_mark ( 5000 );
        
        quicksort ( candlist, 0, cnt-1 );    

        for ( i=supdsloc; i < newdsloc; i++ ) {
            if ( supds[i].sup >= minsup ) {
                spaces += 2;
                mineTrees ( newpat, supds[i].lab, supds[i].position, supds[i].sup, newtidlist, newtid_index, newdsloc, totalcnt, level+1, candlist, cnt );
                spaces -= 2;
            }
        }
    }
    /* if ( tidlist != NULL ) */
    else {
        struct TreeNode *nodelist;
        struct Match *patmatch;
        
        newdsloc = supdsloc;
        for ( tind=0; tind < tidlistsize; tind++ ) {
            tid = tidlist[tind].tid;
            patmatch = treeset[tid].patmatch;
            nodelist = treeset[tid].nodelist;
            
            for ( int ii=supdsloc; ii < newdsloc; ii++ ) 
                supds[ii].flag = 0;

            found = false;
            for ( ind=tidlist[tind].start_loc; ind > 0; ind-- ) {
                parent_ind = nodelist[ind].nps;
                lab_ind = nodelist[ind].labseq;

                /* If its lab_to_add then use it in building the new list 
                   Else, check if it is a growth point */
                   
                bool justfound = false;  // found is made to true in this iteration
                if ( nodelist[ind].labseq == lab_to_add ) {
                    int pos;
                    
                    /* check if parent is valid growth point for lab_to_add */

                    i = tidlist[tind].match_loc - 2;   // match_loc-1 gives the separator
                    while ( i >= 0 && patmatch[i].ptr != -2 ) { // till the separator
                        parent = parent_ind; //treeset[tid].nodelist[ind].nps;
                        bool p_i_p_o_p = false;
                        bool p_f_i_t_m = false; // parent found in this match (i.e., this iteration of i )
                        
                        while(1) {
                            pos = patlength;
                            j = i;
                            do {
                                if (  patmatch[j].matchpon <= ind ) // tmpembed[index] <= ind )
                                    break;
                                
                                if ( parent ==  patmatch[j].matchpon ) { 
                                    p_i_p_o_p = true;  // Parent Is Part Of Pattern
                                    if ( pos == posindex ) {
                                        if ( found == false ) {
                                            found = true;
                                            newtidlist[ newtid_index ].tid = tid;
                                            newtidlist[ newtid_index ].start_loc = ind - 1;
                                            newtid_index++;
                                            match_loc = tidlist[tind].match_loc;
                                        }    
                                        if ( match_loc >= treeset[tid].patmatchlen ) {
                                            treeset[tid].patmatchlen += (PATREALLOC*treeset[tid].nodes);
                                            treeset[tid].patmatch = patmatch = (struct Match *) realloc ( patmatch, 
                                                                                  sizeof(struct Match)*treeset[tid].patmatchlen);
                                        }
                                        patmatch[match_loc].matchpon = ind;
                                        patmatch[match_loc].ptr = j; // i;
                                        match_loc++;
                                        justfound = true;
                                        p_f_i_t_m = true;
                                        break;   
                                    }
                                }
                                pos--;
                                j = patmatch[j].ptr;
                            } while ( j != -1 ); // index < tmploc );
                        
                            // so far we checked parent is part of pattern
                            // now check whether parent of parent is part of pattern
                            // But if we find an exact match (pos and lab) then we shd continue
                            // to check whether the pop is part of some other pattern
                            // ** in mistake.txt
                            if ( p_i_p_o_p == false || p_f_i_t_m == true) {
                                parent = nodelist[parent].nps;
                                if ( parent == 0 || parent > patmatch[0].matchpon )
                                    break;
                            }
                            else
                                break;
                        } // while (1)
                        
                        i--;

                    } // i
                } // if: building new tidlist
                
                bool consider = false;
                int t;
                if ( freqind != NULL ) {
                    t = 1;
                    if ( binarySearch ( freqind, lab_ind, 0, numfreqind-1 ) != -1 )
                        consider = true;
                    else
                        consider = false;
                }
                else {
                    t = 0;
                    if ( labind[lab_ind] >= minsup )
                        consider = true;
                    else 
                        consider = false;
                }

                /* We should try to extend only if we find the match to lab_to_add atleast once */
                if ( found  && consider ) { 
                    int pos;
                    
                    /* Check if parent is valid growth point for lab */

                    // justfound will be true if treeset[tid].labseq[ind] is exact match to lab_to_add.
                    // Then, it can be extension point only to previous matches and hence, match_loc-2
                    if(justfound)
                        i = match_loc - 2;
                    else
                        i = match_loc - 1;
                    
                    while ( i >= 0 && patmatch[i].ptr != -2 ) {
                        parent = parent_ind;
                        lab = lab_ind;
                        bool matchfound = false;  // equivalent of p_i_p_o_p

                        while (1) {
                            pos = patlength + 1;
                            index = 0;
                            j = i;
                            do {
                                /* the label we are looking for is already part of the pattern */
                                if ( patmatch[j].matchpon <= ind )
                                    break;
                                if ( patmatch[j].matchpon == parent ) {
                                
                                    extfound = false;
                                    for ( k=supdsloc; k < newdsloc; k++ ) {
                                        if ( supds[k].lab == lab && supds[k].position == pos ) { // pos should be index into pattern !!!!
                                            extfound = true;
                                            if( supds[k].flag == 0 ) {
                                                supds[k].sup++;
                                                supds[k].flag = 1;
                                            }
                                            break;    
                                        }
                                    }
                                    if ( extfound == false ) {
                                        if ( newdsloc + 1 > supsize ) {
                                            supsize += SUPREALLOC;
                                            supds = (SupportStructure *) realloc ( supds, sizeof(SupportStructure) * supsize );
                                        }
                                        supds[newdsloc].lab = lab;
                                        supds[newdsloc].position = pos;
                                        supds[newdsloc].sup = 1;
                                        supds[newdsloc].flag = 1;
                                        newdsloc++;
                                        /* Though we found a match, we should not break.. 
                                           There might be extensions at a different position with same parent */
                                    }
                                    matchfound = true;
                                } 
                                pos--;
                                j = patmatch[j].ptr;
                            } while ( j!= -1 );

                            // so far we checked whether parent is part of pattern.
                            // Now check if parent of parent (pop) is part of pattern
                            // We check for pop ONLY IF parent is NOT part of pattern
                            // i.e., we break as soon as we find the position at which lab can be added
                            // i.e., we are looking for the first match.
    
                            if ( matchfound == false ) {
                                parent = nodelist[parent].nps;
                                if ( parent == 0 || parent > patmatch[0].matchpon )
                                    break;
                            }
                            else
                                break;
                        } //while(1);
                    
                        i--;
                        
                    } // i
                }  // growth point check
            } // ind
            
            if ( found == true ) {
                if ( match_loc >= treeset[tid].patmatchlen ) {
                        treeset[tid].patmatchlen += (PATREALLOC*treeset[tid].nodes);
                        treeset[tid].patmatch = patmatch = (struct Match *) realloc ( patmatch, 
                                                                                  sizeof(struct Match)*treeset[tid].patmatchlen);
                }
                treeset[tid].patmatch [ match_loc ].matchpon = 0;
                treeset[tid].patmatch [ match_loc ].ptr = -2;
                match_loc++;
                newtidlist[ newtid_index - 1 ].match_loc = match_loc;
            }
        } // tind
        
        LABEL *candlist = (LABEL *) membuf.newbuf ( sizeof(LABEL) * (newdsloc-supdsloc+1) );
        // Assuming 5000 th recursion never occurs
        membuf.mark_buf ( 5000 );
        
        // true if it is candidate frequent item ..
        bool *labflag = (bool *) membuf.newbuf ( sizeof(bool) * (F1+1));
        for ( int ii=0; ii <= F1; ii++)
            labflag[ii] = false;

        int cnt =0;
        for ( i=supdsloc; i < newdsloc; i++ ) {
            if ( supds[i].sup >= minsup ) {
                if ( labflag[ supds[i].lab ] == false ) {
                    labflag[ supds[i].lab ] = true;
                    candlist[cnt] = supds[i].lab;
                    cnt++;
                }
            }
        }
        
        membuf.clear_to_mark ( 5000 );
        
        quicksort ( candlist, 0, cnt-1 );    

        // Mine the extensions
        for ( i=supdsloc; i < newdsloc; i++ ) {
            if ( supds[i].sup >= minsup ) {
                spaces += 2;
                mineTrees ( newpat, supds[i].lab, supds[i].position, supds[i].sup, newtidlist, newtid_index, newdsloc, totalcnt, level+1, candlist, cnt );
                spaces -= 2;
            }
        }
    }

    membuf.clear_to_mark ( level );

}// mineTrees

/* -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ */

// Increasing Order

void quicksort(int x[], int first, int last) {
    int pivIndex = 0;
    if(first < last) {
        pivIndex = partition(x,first, last);
        quicksort(x,first,(pivIndex-1));
        quicksort(x,(pivIndex+1),last);
    }
}

int partition(int y[], int f, int l) {
    int up,down,temp;
    int piv = y[f];
    up = f;
    down = l;
    goto partLS;
    do { 
        temp = y[up];
        y[up] = y[down];
        y[down] = temp;
    partLS:
        while (y[up] <= piv && up < l) {
            up++;
        }
        while (y[down] > piv  && down > f ) {
            down--;
        }
    } while (down > up);
    y[f] = y[down];
    y[down] = piv;
    return down;
}

int binarySearch(LABEL *a, LABEL value, int left, int right) {
    int mid;
    while ( left <= right ) {
        mid = ((right-left)/2) + left;
        if (a[mid] == value)
            return mid;
        if (value < a[mid])
            right = mid-1;
        else
            left  = mid+1;
    }
    return -1;
}


/* -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ */

/*
Input Format:
    # of trees
    <#nodes> <NPS of tree 1>
    <#nodes> <Label seq of tree 1>
    <#nodes> <NPS of tree 2>
    <#nodes> <Label seq of tree 2>
    ...
*/

void readTrees ( char *file ) {
COUNT db, nodes;
PON num;
LABEL lnum, maxlab = 0;
FILE *fp;
char c;

    fp = fopen ( file, "r" );
    if ( !fp ) {
        fprintf(stderr, "Cannot open the file: %s\n", file );
        exit(1);
    }

    db = 0;
    c = fgetc(fp);
    while ( c >= '0' && c <= '9' ) {
        db = (db*10) + (c - '0');
        c = fgetc(fp);
    }
    Tree::dbsize = db;

    flags = (long *) malloc ( sizeof(long) * (MAXLAB+1) );
    for ( int i=0; i <= MAXLAB; i++ )
        flags[i] = -1;

    treeset = (Tree *) malloc ( sizeof(Tree) * db );
    maxnodes = 0;
    for ( int tid=0; tid < db; tid++ ){
        
        /* Read the NPS of the tree */
        
        nodes = 0;
        c = fgetc(fp);
        while ( c >= '0' && c <= '9' ) {
            nodes = (nodes*10) + (c - '0');
            c = fgetc(fp);
        }
        if ( maxnodes < nodes ) 
            maxnodes = nodes;
            
        treeset[tid].nodes = nodes;
        treeset[tid].nodelist = (struct TreeNode *) malloc ( sizeof(struct TreeNode) * (nodes+1) );
        
        for ( int nodeid=1; nodeid <= nodes; nodeid++ ) {
            num = 0;
            c = fgetc(fp);
            while ( c >= '0'&& c <= '9' ) {
                num = (num*10) + (c - '0');
                c = fgetc(fp);
            }
            treeset[tid].nodelist[nodeid].nps = num;
        }
        treeset[tid].nodelist[0].nps = -1;
        
        /* Read the Label Sequence of the tree */
        
        nodes = 0;
        c = fgetc(fp);
        while ( c >= '0' && c <= '9' ) {
            nodes = (nodes*10) + (c - '0');
            c = fgetc(fp);
        }
        
        for ( int nodeid=1; nodeid <= nodes; nodeid++ ) {
            lnum = 0;
            c = fgetc(fp);
            while ( c >= '0'&& c <= '9' ) {
                lnum = (lnum*10) + (c - '0');
                c = fgetc(fp);
            }
            if ( maxlab < lnum ) 
                maxlab = lnum;
            if ( flags[lnum] != tid ) {
                labind[lnum]++;
                flags[lnum] = tid;
            }
            treeset[tid].nodelist[nodeid].labseq = lnum;
        }
        treeset[tid].nodelist[0].labseq = -1;
    } // tid

    maxlab++;
    labcount = maxlab;
    
    fclose(fp);
    free(flags);
    
} // readTrees

/* -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~ */

