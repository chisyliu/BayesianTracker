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

#include "hypothesis.h"



// safe log function
double safe_log(double value)
{
  if (value <= 0.) return std::log(DEFAULT_LOW_PROBABILITY);
  return std::log(value);
};



// calculate the Euclidean distance between the end of one track and the
// beginning of the second track
double link_distance( const TrackletPtr a_trk,
                      const TrackletPtr a_trk_lnk )
{
  Eigen::Vector3d d = a_trk->track.back()->position()
                      - a_trk_lnk->track.front()->position();
  return std::sqrt( d.transpose()*d );
}



// calculate the Euclidean distance between the end of one track and the
// beginning of the second track
double link_time( const TrackletPtr a_trk,
                  const TrackletPtr a_trk_lnk )
{
  return a_trk_lnk->track.front()->t - a_trk->track.back()->t;
}



// count the number of apoptosis events, starting at the terminus of the
// track
unsigned int count_apoptosis(const TrackletPtr a_trk)
{

  // check that we have at least one observation in our track
  assert(a_trk->length()>0);

  int counter = a_trk->length()-1;
  unsigned int n_apoptosis = 0;

  // NOTE(arl): could just count the number of apoptotic states?
  // for (size_t i=0; i<trk->length(); i++){
  //   if (trk->track[i]->label == STATE_apoptosis) n_apoptosis+=1;
  // }

  while (a_trk->track[counter]->label == STATE_apoptosis &&
         counter>=0) {

    // increment the number of apoptoses
    n_apoptosis++;
    counter--;

    // immediately exit if we have reached the end
    if (counter<0) break;
  }

  return n_apoptosis;
}









HypothesisEngine::HypothesisEngine( void )
{
  // default constructor
}



HypothesisEngine::HypothesisEngine( const unsigned int a_start_frame,
                                    const unsigned int a_stop_frame,
                                    const PyHypothesisParams& a_params )
{
  //m_num_frames = a_stop_frame - a_start_frame;
  m_frame_range[0] = a_start_frame;
  m_frame_range[1] = a_stop_frame;
  m_params = a_params;

  // tell the user which hypotheses are going to be created
  // ['P_FP','P_init','P_term','P_link','P_branch','P_dead','P_merge']

  if (DEBUG) {
    std::cout << "Hypotheses to generate: " << std::endl;
    std::cout << " - P_FP: " << hypothesis_allowed(TYPE_Pfalse) << std::endl;
    std::cout << " - P_init: " << hypothesis_allowed(TYPE_Pinit) << std::endl;
    std::cout << " - P_term: " << hypothesis_allowed(TYPE_Pterm) << std::endl;
    std::cout << " - P_link: " << hypothesis_allowed(TYPE_Plink) << std::endl;
    std::cout << " - P_branch: " << hypothesis_allowed(TYPE_Pdivn) << std::endl;
    std::cout << " - P_dead: " << hypothesis_allowed(TYPE_Papop) << std::endl;
    std::cout << " - P_merge: " << hypothesis_allowed(TYPE_Pmrge) << std::endl;
  }

  // set up a HashCube
  m_cube = HypercubeBin(m_params.dist_thresh, m_params.time_thresh);
}



HypothesisEngine::~HypothesisEngine( void )
{
  // default destructor
  m_tracks.clear();
}




void HypothesisEngine::add_track(TrackletPtr a_trk)
{
  // push this onto the list of trajectories
  m_tracks.push_back( a_trk );

  // add this one to the hash cube
  m_cube.add_tracklet( a_trk );
}




// test whether to generate a certain type of hypothesis
bool HypothesisEngine::hypothesis_allowed(const unsigned int a_hypothesis_type) const
{

  //TODO(arl): make sure that the type exists!
  unsigned int bitmask = std::pow(2, a_hypothesis_type);

  //std::cout << a_hypothesis_type << "," << bitmask << std::endl;
  if ((m_params.hypotheses_to_generate & bitmask) == bitmask) {
    return true;
  }

  return false;
}



float HypothesisEngine::dist_from_border( TrackletPtr a_trk,
                                          bool a_start=true ) const
{
  // Calculate the distance from the border of the field of view
  float min_dist, min_this_dim;
  Eigen::Vector3d xyz;

  // take either the first or last localisation of the object
  if (a_start) {
    xyz = a_trk->track.front()->position();
  } else {
    xyz = a_trk->track.back()->position();
  }

  // set the distance to infinity
  min_dist = kInfinity;

  // find the smallest distance between a point and the edge of the volume
  // NOTE(arl): what if we have zero dimensions?
  for (unsigned short dim=0; dim<3; dim++) {

    // skip a dimension if it does not exist
    if (volume.min_xyz[dim] == volume.max_xyz[dim]) continue;

    min_this_dim = std::min(xyz[dim]-volume.min_xyz[dim],
                            volume.max_xyz[dim]-xyz[dim]);

    if (min_this_dim < min_dist) {
      min_dist = min_this_dim;
    }
  }

  return min_dist;
}



// create the hypotheses
void HypothesisEngine::create( void )
{

  if (m_tracks.size() < 1) return;

  // get the tracks
  m_num_tracks = m_tracks.size();

  // reserve some memory for the hypotheses (at least 5 times the number of
  // trajectories)
  m_hypotheses.reserve( m_num_tracks*5 );

  TrackletPtr trk;

  // loop through trajectories
  for (size_t i=0; i<m_num_tracks; i++) {

    // get the test track
    trk = m_tracks[i];

    // false positive hypothesis calculated for everything
    Hypothesis h_fp(TYPE_Pfalse, trk);
    h_fp.probability = safe_log( P_FP( trk ) );
    m_hypotheses.push_back( h_fp );

    // distance from the frame border
    float d_start = dist_from_border( trk, true );
    float d_stop = dist_from_border( trk, false );

    // now calculate the initialisation
    if (hypothesis_allowed(TYPE_Pinit)) {
      if (m_params.relax ||
          trk->track.front()->t < m_frame_range[0]+m_params.theta_time ||
          d_start < m_params.theta_dist ) {

        Hypothesis h_init(TYPE_Pinit, trk);
        h_init.probability = safe_log(P_init(trk)) + 0.5*safe_log(P_TP(trk));
        m_hypotheses.push_back( h_init );
      }
    }

    // termination?
    if (hypothesis_allowed(TYPE_Pterm)) {
      if (m_params.relax ||
          trk->track.back()->t > m_frame_range[1]-m_params.theta_time ||
          d_stop < m_params.theta_dist) {

        Hypothesis h_term(TYPE_Pterm, trk);
        h_term.probability = safe_log(P_term(trk)) + 0.5*safe_log(P_TP(trk));
        m_hypotheses.push_back( h_term );
      }
    }

    // NEW apoptosis detection hypothesis
    // modify this for apoptosis
    unsigned int n_apoptosis = count_apoptosis(trk);

    if (hypothesis_allowed(TYPE_Papop) &&
        n_apoptosis > m_params.apop_thresh) {

      Hypothesis h_apoptosis(TYPE_Papop, trk);
      h_apoptosis.probability = safe_log(P_dead(trk, n_apoptosis))
                                + 0.5*safe_log(P_TP(trk));
      m_hypotheses.push_back( h_apoptosis );
    }

    // manage conflicts
    std::vector<TrackletPtr> conflicts;

    // iterate over all of the tracks in the hash cube
    std::vector<TrackletPtr> trks_to_test = m_cube.get( trk, false );

    for (size_t j=0; j<trks_to_test.size(); j++) {
      // get the track
      TrackletPtr this_trk = trks_to_test[j];

      float d = link_distance(trk, this_trk);
      float dt = link_time(trk, this_trk);

      // if we exceed these continue
      if (d  >= m_params.dist_thresh) continue;
      if (dt >= m_params.time_thresh || dt < 1) continue; // this was one

      // TODO(arl): limits the maximum link distance ?
      if (hypothesis_allowed(TYPE_Plink)) {

        Hypothesis h_link(TYPE_Plink, trk);
        h_link.trk_link_ID = this_trk;
        h_link.probability = safe_log(P_link(trk, this_trk, d, dt))
                            + 0.5*safe_log(P_TP(trk))
                            + 0.5*safe_log(P_TP(this_trk));
        m_hypotheses.push_back( h_link );
      }

      // append this to conflicts
      conflicts.push_back( this_trk );

    } // j

    // if we have conflicts, this may mean divisions have occurred
    if (conflicts.size() < 2) continue;

    // iterate through the conflicts and put division hypotheses into the
    // list, including links to the children
    for (unsigned int p=0; p<conflicts.size(); p++) {
      // get the first putative child
      TrackletPtr child_one = conflicts[p];

      for (unsigned int q=p+1; q<conflicts.size(); q++) {
        // get the second putative child
        TrackletPtr child_two = conflicts[q];

        if (hypothesis_allowed(TYPE_Pdivn)) {

          Hypothesis h_divn(TYPE_Pdivn, trk);
          h_divn.trk_child_one_ID = child_one;
          h_divn.trk_child_two_ID = child_two;
          h_divn.probability = safe_log(P_branch(trk, child_one, child_two))
                              + 0.5*safe_log(P_TP(trk))
                              + 0.5*safe_log(P_TP(child_one))
                              + 0.5*safe_log(P_TP(child_two));
          m_hypotheses.push_back( h_divn );
        }
      } // q
    } // p

  }

}



// FALSE POSITIVE TRAJECTORY
double HypothesisEngine::P_FP( TrackletPtr a_trk ) const
{
  unsigned int len_track = static_cast<unsigned int>(1.+a_trk->duration());
  return std::pow(m_params.segmentation_miss_rate, len_track);
}



// TRUE POSITIVE TRAJECTORY
double HypothesisEngine::P_TP( TrackletPtr a_trk ) const
{
  return 1.0 - P_FP(a_trk);
}



// INITIALISATION PROBABILITY
double HypothesisEngine::P_init( TrackletPtr a_trk ) const
{
  // Probability of a true initialisation event.  These tend to occur close to
  // the beginning of the sequence or at the periphery of the field of view as
  // objects enter.


  float dist = dist_from_border(a_trk, true);

  double prob[2] = {0.0, 0.0};
  bool init = false;


  if (a_trk->track.front()->t < m_frame_range[0]+m_params.theta_time) {
    prob[0] = std::exp(-(a_trk->track.front()->t-(float)m_frame_range[0]+1.0) /
              m_params.lambda_time);
    init = true;
  }

  if ( dist < m_params.theta_dist || m_params.relax ) {
    prob[1] = std::exp(-dist/m_params.lambda_dist);
    init = true;
  }

  if (init) {
    return std::max(prob[0], prob[1]);
  }

  // default return
  return m_params.eta;
}



// TERMINATION PROBABILITY
double HypothesisEngine::P_term( TrackletPtr a_trk ) const
{
  // Probability of termination event.  Similar to initialisation, except that
  // we use the final location/time of the tracklet.

  float dist = dist_from_border(a_trk, false);

  double prob[2] = {0.0, 0.0};
  bool term = false;

  if (m_frame_range[1]-a_trk->track.back()->t < m_params.theta_time) {
    prob[0] = std::exp(-((float)m_frame_range[1]-a_trk->track.back()->t) /
              m_params.lambda_time );
    term = true;
  }

  if ( dist < m_params.theta_dist || m_params.relax ) {
    prob[1] = std::exp(-dist/m_params.lambda_dist);
    term = true;
  }

  if (term) {
    return std::max(prob[0], prob[1]);
  }

  // default return
  return m_params.eta;

}



// APOPTOSIS PROBABILITY
double HypothesisEngine::P_dead(TrackletPtr a_trk,
                                const unsigned int n_apoptosis ) const
{
  // want to discount this by how close it is to the border of the field of
  // view
  float dist = dist_from_border(a_trk, false);
  float discount = 1.0 - std::exp(-dist/m_params.lambda_dist);
  return (1.0 - std::pow(m_params.apoptosis_rate, n_apoptosis)) * discount;
}

double HypothesisEngine::P_dead( TrackletPtr a_trk ) const
{
  unsigned int n_apoptosis = count_apoptosis(a_trk);
  return P_dead(a_trk, n_apoptosis);
}



// LINKING PROBABILITY
double HypothesisEngine::P_link(TrackletPtr a_trk,
                                TrackletPtr a_trk_lnk) const
{
  float d = link_distance(a_trk, a_trk_lnk);
  float dt = link_time(a_trk, a_trk_lnk);

  // return the full version output
  return P_link(a_trk, a_trk_lnk, d, dt);
}

double HypothesisEngine::P_link(TrackletPtr a_trk,
                                TrackletPtr a_trk_lnk,
                                float d,
                                float dt) const
{

  // try to not link metaphase to anaphase
  if (DISALLOW_METAPHASE_ANAPHASE_LINKING) {
    if (a_trk->track.back()->label == STATE_metaphase &&
        a_trk_lnk->track.front()->label == STATE_anaphase) {
      return m_params.eta ;
    }
  }
  // make sure that we're looking forward in time
  assert(dt>0.0);

  // DONE(arl): need to penalise longer times between tracks, dt acts as
  // a linear scaling penalty - scale the distance linearly by time
  return std::exp(-(d*dt)/m_params.lambda_link);
}



// DIVISION PROBABILITY
double HypothesisEngine::P_branch(TrackletPtr a_trk,
                                  TrackletPtr a_trk_c0,
                                  TrackletPtr a_trk_c1) const
{

  // calculate the distance between the previous observation and both of the
  // putative children these are the normalising factors for the dot product
  // a dot product < 0 would indicate that the cells are aligned with the
  // metaphase phase i.e. a good division
  Eigen::Vector3d d_c0, d_c1;
  d_c0 = a_trk_c0->track.front()->position() - a_trk->track.back()->position();
  d_c1 = a_trk_c1->track.front()->position() - a_trk->track.back()->position();

  // normalise the vectors to calculate the dot product
  double dot_product = d_c0.normalized().transpose() * d_c1.normalized();

  // initialise variables
  double delta_g;
  double weight;

  // parent is metaphase
  if (a_trk->track.back()->label == STATE_metaphase) {
    if (a_trk_c0->track.front()->label == STATE_anaphase &&
        a_trk_c1->track.front()->label == STATE_anaphase) {

        // BEST
        weight = WEIGHT_METAPHASE_ANAPHASE_ANAPHASE;
    } else if ( a_trk_c0->track.front()->label == STATE_anaphase ||
                a_trk_c1->track.front()->label == STATE_anaphase ) {

        // PRETTY GOOD
        weight = WEIGHT_METAPHASE_ANAPHASE;
    } else {

      // OK
      weight = WEIGHT_METAPHASE;
    }

  // parent is not metaphase
  } else {
    if (a_trk_c0->track.front()->label == STATE_anaphase &&
        a_trk_c1->track.front()->label == STATE_anaphase) {

          // PRETTY GOOD
          weight = WEIGHT_ANAPHASE_ANAPHASE;
    } else if ( a_trk_c0->track.front()->label == STATE_anaphase ||
                a_trk_c1->track.front()->label == STATE_anaphase ) {

          // OK
          weight = WEIGHT_ANAPHASE;
    } else {
      // in this case, none of the criteria are satisfied
      weight = WEIGHT_OTHER + 10.*P_dead(a_trk_c0) + 10.*P_dead(a_trk_c1);
    }
  }


  // weighted angle between the daughter cells and the parent
  // use an erf as the weighting function
  // dot product scales between -1 (ideal) where the daughters are on opposite
  // sides of the parent, to 1, where the daughters are close in space on the
  // same side (worst case). Error function will scale these from ~0. to ~1.
  // meaning that the ideal case minimises the delta_g
  delta_g = weight * ((1.-std::erf(dot_product / (3.*kRootTwo)))/2.0);

  return std::exp(-delta_g/(2.*m_params.lambda_branch));
}
