# Amazon-MIT-RoutingChallenge
Implementation of the **pool and select** framework for solving the Amazon/MIT routing challenge.

Authors: Alexandre Florio, Paulo da Costa, Sami Serkan Özarık.

## Description
This project contains all code developed by our group *TurkishChurrasco* for solving the [Amazon/MIT Routing Challenge](https://routingchallenge.mit.edu).

### Context
Optimization of last-mile delivery routes is a complex operational task in transportation logistics. Many factors must be taken into account when designing efficient routes, including the total distance traveled by drivers, customer preferences and time windows, availability of parking spaces and spatiotemporal congestion patterns.

The **pool and select** framework is a machine learning-based framework for optimizing last-mile delivery routes. In the *pool* phase, a large number of candidate delivery sequences is generated based on structural information acquired from training data. In the *select* phase, the score of each candidate sequence is evaluated with a pre-trained and regularized regression model, and the best (predicted) sequence is returned.

### Reference
For more details on the framework and this implementation, please refer to:

**Florio, A.M., da Costa, P., & Özarık, S.S. (2021). A Machine Learning Framework for Last-Mile Delivery Optimization. In Winkenbach, M., Parks, S., & Noszek, J. (Eds.), Technical Proceedings of the 2021 Amazon Last Mile Routing Research Challenge (pp. XXI.1–XXI.12). MIT Libraries. [https://hdl.handle.net/1721.1/131235](https://hdl.handle.net/1721.1/131235)**

## Dependencies
The implementation requires:
* The `rapidjson` ([https://rapidjson.org](https://rapidjson.org)) XML parser;
* A `date.h` implementation (e.g., [this one](https://github.com/HowardHinnant/date/blob/master/include/date/date.h));
* The [mlpack](https://www.mlpack.org) library for training the score prediction models by Lasso regression.
