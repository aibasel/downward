;; Problem to test issue75. If we add the implied preconditions, the
;; preprocessor cannot prune any variables; if we do, it can prune lots.
;;
;; This is based on Trucks-Strips #1, with the following modifications:
;;
;; - Removed everything pertaining to packages 2 and 3.

(define (problem GROUNDED-TRUCK-1)
(:domain GROUNDED-TRUCKS)
(:init
(FOO)
(time-now_t0)
(at_package1_l2)
(free_a2_truck1)
(free_a1_truck1)
(at_truck1_l3)
)
(:goal
(and
(delivered_package1_l3_t3)
)
)
)