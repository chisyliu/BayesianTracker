/*
--------------------------------------------------------------------------------
 Name:     BayesianTracker
 Purpose:  A multi object tracking library, specifically used to reconstruct
           tracks in crowded fields. Here we use a probabilistic network of
           information to perform the trajectory linking. This method uses
           positional and visual information for track linking.

 Authors:  Alan R. Lowe (arl) a.lowe@ucl.ac.uk

 License:  See LICENSE.md

 Created:  14/08/2014
--------------------------------------------------------------------------------
*/

#ifndef _DEFS_H_INCLUDED_
#define _DEFS_H_INCLUDED_


#include <limits>

// errors
#define SUCCESS 900
#define ERROR_empty_queue 901
#define ERROR_no_tracks 902
#define ERROR_no_useable_frames 903
#define ERROR_track_empty 904
#define ERROR_incorrect_motion_model 905
#define ERROR_max_lost_out_of_range 906
#define ERROR_accuracy_out_of_range 907
#define ERROR_prob_not_assign_out_of_range 908
#define ERROR_not_defined 909
#define ERROR_none 910

// constants
const double kInfinity = std::numeric_limits<double>::infinity();

// constants for integrating Gaussian PDF
const double kRootTwo = std::sqrt(2.0);
const double kRootTwoPi = std::sqrt(2.0*M_PI);

// tracking params
#define PROB_NOT_ASSIGN 0.01
#define DEFAULT_ACCURACY 2.0
#define DISALLOW_METAPHASE_ANAPHASE_LINKING true
#define PROB_ASSIGN_EXP_DECAY true
#define DYNAMIC_ACCURACY false
#define DIMS 3
//#define FAST_COST_UPDATE false
#define MAX_LOST 5
#define MAX_SEARCH_RADIUS 10


// reserve space for objects and tracks
#define RESERVE_NEW_OBJECTS 1000
#define RESERVE_ACTIVE_TRACKS 1000

// possible states of objects
#define STATE_interphase 0
#define STATE_prometaphase 1
#define STATE_metaphase 2
#define STATE_anaphase 3
#define STATE_apoptosis 4
#define STATE_null 5

// hypothesis and state types
// ['P_FP','P_init','P_term','P_link','P_branch','P_dead','P_merge']
#define TYPE_Pfalse 0
#define TYPE_Pinit 1
#define TYPE_Pterm 2
#define TYPE_Plink 3
#define TYPE_Pdivn 4
#define TYPE_Papop 5
#define TYPE_Pmrge 6
#define TYPE_Pdead 666
#define TYPE_undef 999


// hypothesis generation
#define MAX_TRACK_LEN 150
#define DEFAULT_LOW_PROBABILITY 1e-308

#define WEIGHT_METAPHASE_ANAPHASE_ANAPHASE 0.01
#define WEIGHT_METAPHASE_ANAPHASE 0.1
#define WEIGHT_METAPHASE 2.0
#define WEIGHT_ANAPHASE_ANAPHASE 1.0
#define WEIGHT_ANAPHASE 2.0
#define WEIGHT_OTHER 5.0


#endif
