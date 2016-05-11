(define (domain miconic)
  (:requirements :strips)
  

(:predicates 
(origin ?person ?floor )
;; entry of ?person is ?floor
;; inertia

(floor ?floor)
(passenger ?passenger)

(destin ?person ?floor )
;; exit of ?person is ?floor
;; inertia

(above ?floor1 ?floor2 )
;; ?floor2 is located above of ?floor1

(boarded ?person )
;; true if ?person has boarded the lift

(served ?person )
;; true if ?person has alighted as her destination

(lift-at ?floor )
;; current position of the lift is at ?floor
)


;;stop and allow boarding

(:action board
  :parameters (?f ?p)
  :precondition (and (floor ?f) (passenger ?p)(lift-at ?f) (origin ?p ?f))
  :effect (boarded ?p))

(:action depart
  :parameters (?f  ?p)
  :precondition (and (floor ?f) (passenger ?p) (lift-at ?f) (destin ?p ?f)
		     (boarded ?p))
  :effect (and (not (boarded ?p))
	       (served ?p)))
;;drive up

(:action up
  :parameters (?f1 ?f2)
  :precondition (and (floor ?f1) (floor ?f2) (lift-at ?f1) (above ?f1 ?f2))
  :effect (and (lift-at ?f2) (not (lift-at ?f1))))


;;drive down

(:action down
  :parameters (?f1 ?f2)
  :precondition (and (floor ?f1) (floor ?f2) (lift-at ?f1) (above ?f2 ?f1))
  :effect (and (lift-at ?f2) (not (lift-at ?f1))))
)



