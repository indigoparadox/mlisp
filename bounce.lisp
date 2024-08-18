
(begin
   (define fly
      (lambda (x y vx vy)
         (rect x y vx vy)
         (define vx
            (if (gt (+ x vx) 310) -1 1))
         (define vx
            (if (gt (+ y vy) 190) -1 1))
         (fly (+ x vx) (+ y vy) vx vy)))
   (fly 0 0 1 1))
