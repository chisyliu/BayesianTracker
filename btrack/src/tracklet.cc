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

#include "tracklet.h"



Tracklet::Tracklet( const unsigned int new_ID,
                    const TrackObjectPtr& new_object,
                    const unsigned int max_lost,
                    const MotionModel& model ) {

  // make a local copy of the default motion model
  motion_model = model;

  // set the current state to the position of the object
  motion_model.setup( new_object );

  // make sure we set the remove flag to false
  remove_flag = false;

  // starting a new tracklet
  ID = new_ID;
  append( new_object, false );

}



void Tracklet::append(const TrackObjectPtr& new_object, bool update) {

  // append the new object to the end of the track
  track.push_back( new_object );

  // update the motion model and the prediction
  if (update) {
    motion_model.update( new_object );
  }

  // push back the prediction before the update
  prediction.push_back( this->predict() );
  // store the Kalman filter output
  kalman.push_back( this->motion_model.predict() );

  // if this is a dummy object increment the lost counter
  if (new_object->dummy) {
    lost++;
  } else {
    lost = 0; // reset this so that we don't accumulate without successive dummy
  }

}



// append a dummy object to the tracklet
void Tracklet::append_dummy() {
  if (lost >= MAX_LOST)
    return;

  // get the predicted new position
  Prediction p = this->predict();

  // make a dummy track object by copying the last observation
  TrackObjectPtr dummy = std::make_shared<TrackObject>( *(this->track.back()) );
  dummy->dummy = true;
  dummy->x = p.mu(0);
  dummy->y = p.mu(1);
  dummy->z = p.mu(2);
  dummy->t = dummy->t+1; // NOTE(arl): is this valid assumption?
  dummy->ID = 0;

  // append the dummy to the track
  this->append( dummy );
}



// Trim the tracklet to remove any dummy objects if the track has been lost
bool Tracklet::trim() {
  while (track.back()->dummy) {
    track.pop_back();
  }
  return true;
}



// make a prediction about the future state of the tracklet
// TODO(arl): make this model agnostic
Prediction Tracklet::predict() const {
  Prediction p_out;
  Prediction p = motion_model.predict();
  //p_out.mu = position() + p.mu.tail(3); // add the displacement vector
  p_out.mu = position() + motion_model.get_motion_vector();
  p_out.covar = p.covar.block(0,0,3,3);
  return p_out;
}
