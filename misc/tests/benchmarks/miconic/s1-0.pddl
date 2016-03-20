


(define (problem mixed-f2-p1-u0-v0-g0-a0-n0-A0-B0-N0-F0-r0)
   (:domain miconic)
   (:objects p0 
             f0 f1 )


(:init
(passenger p0)
(floor f0)
(floor f1)

(above f0 f1)
(origin p0 f1)
(destin p0 f0)

(lift-at f0)
)


(:goal (and 
(served p0)
))
)


