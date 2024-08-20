
(begin
   (define fly
      (lambda (x y vx vy)
         (rectf black 0 0 320 200)
         (rect blue x y 10 10)
         (define vx
            (if 
               (or (> (+ x vx) 310) (< (+ x vx) 0))
               (* vx -1) vx))
         (define vy
            (if
               (or (> (+ y vy) 190) (< (+ y vy) 0))
               (* vy -1) vy))
         (fly (+ x vx) (+ y vy) vx vy)))
   (fly 0 0 10 10))

