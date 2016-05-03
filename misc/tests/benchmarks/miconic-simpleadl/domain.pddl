(define (domain miconic)
  (:requirements :adl :typing)
  (:types passenger - object
          floor - object
         )

(:predicates 
(origin ?person - passenger ?floor - floor)
;; entry of ?person is ?floor
;; inertia

(destin ?person - passenger ?floor - floor)
;; exit of ?person is ?floor
;; inertia

(above ?floor1 - floor  ?floor2 - floor)
;; ?floor2 is located above of ?floor1

(boarded ?person - passenger)
;; true if ?person has boarded the lift

(served ?person - passenger)
;; true if ?person has alighted as her destination

(lift-at ?floor - floor)
;; current position of the lift is at ?floor
)


;;stop

(:action stop
  :parameters (?f - floor)
  :precondition (lift-at ?f)
  :effect (and 
               (forall (?p - passenger) 
                  (when (and (boarded ?p) 
                             (destin ?p ?f))
                        (and (not (boarded ?p)) 
                             (served  ?p))))
               (forall (?p - passenger)                
                   (when (and (origin ?p ?f) (not (served ?p)))
                              (boarded ?p)))))


;;drive up

(:action up
  :parameters (?f1 - floor ?f2 - floor)
  :precondition (and (lift-at ?f1) (above ?f1 ?f2))
  :effect (and (lift-at ?f2) (not (lift-at ?f1))))


;;drive down

(:action down
  :parameters (?f1 - floor ?f2 - floor)
  :precondition (and (lift-at ?f1) (above ?f2 ?f1))
  :effect (and (lift-at ?f2) (not (lift-at ?f1))))
)



