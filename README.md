# Bayesian Tracker

** WORK IN PROGRESS ** (Last update: 14/09/2017)


BayesianTracker is a multi object tracking algorithm, specifically used to
reconstruct trajectories in crowded fields. Here, we use a probabilistic network
of information to perform the trajectory linking. This method uses spatial
information as well as appearance information for track linking.

The tracking algorithm assembles reliable sections of track that do not
contain splitting events (tracklets). Each new tracklet initiates a
probabilistic model, and utilises this to predict future states (and error in
states) of each of the objects in the field of view.  We assign new observations
to the growing tracklets (linking) by evaluating the posterior probability of
each potential linkage from a Bayesian belief matrix for all possible linkages.

[![SquiggleBox](http://lowe.cs.ucl.ac.uk/images/tracks.png)]()
*Example of tracking objects in 3D space*

BayesianTracker (btrack) is part of the ImPy image processing toolbox for
microscopy data analysis. For more information see: http://lowe.cs.ucl.ac.uk/

---

### Dependencies

BayesianTracker has been tested with Python 2.7+ on OS X and Linux, and requires
the following additional packages:

+ Numpy
+ Scipy
+ h5py
+ Eigen

---

### Examples

We developed BayesianTracker to enable us to track individual molecules or
cells in large populations over very long periods of time, reconstruct lineages
and study cell movement or sub-cellular protein localisation. We have provided
several examples in the notebooks folder.  Below is an example of tracking
cells:

[![CellTracking](http://lowe.cs.ucl.ac.uk/images/youtube.png)](https://youtu.be/dsjUnRwu33k)

More details of how the tracking algorithm can be applied to tracking cells in
time-lapse microscopy data can be found in our publication:

**Local cellular neighbourhood controls proliferation in cell competition**  
Bove A, Gradeci D, Fujita Y, Banerjee S, Charras G and Lowe AR.  
*Mol. Biol. Cell* (2017) <https://doi.org/10.1091/mbc.E17-06-0368>

---

### Installation

You can install BayesianTracker by cloning the repo and running the setup:
```sh
$ git clone https://github.com/quantumjot/BayesianTracker.git
$ cd BayesianTracker
$ python setup.py install
```

---

### Usage

BayesianTracker can be used simply as follows:

```python
import btrack

# NOTE:  This should be from your image segmentation code
objects = [btrack.PyTrackObject(t) for t in observations]

# initialise a tracker session using a context manager
with btrack.BayesianTracker() as tracker:

  # append the objects to be tracked
  tracker.append(objects)

  # track them
  tracker.track()

  # get the tracks
  track_zero = tracker[0]
  tracks = tracker.tracks

  # export them
  tracker.export('/home/arl/Documents/tracks.json')
```

There are many additional options, including the ability to define object models.

### Input data
Observations can be provided in three basic formats:
+ a simple JSON file
+ HDF5 for larger/more complex datasets, or
+ using your own code as a PyTrackObject.

Here is an example of the JSON format:
```json
{
  "Object_203622": {
    "x": 554.29737483861709,
    "y": 1199.362071438818,
    "z": 0.0,
    "t": 862,
    "label": "interphase",
    "states": 5,
    "probability": [
      0.996992826461792,
      0.0021888131741434336,
      0.0006106126820668578,
      0.000165432647918351,
      4.232166247675195e-05
    ],
    "dummy": false
  }
}
```


### Motion models
Motion models can be written as simple JSON files which can be imported into the tracker.

```json
{
  "MotionModel":{
    "name": "Example",
    "dt": 1.0,
    "measurements": 1,
    "states": 3,
    "accuracy": 2.0,
    "A": {
      "matrix": [1,0,0]
       }
    }
}
```

Or can be built using the MotionModel class.



### Object models
To be completed.