! RUN: %python %S/test_errors.py %s %flang_fc1 -pedantic
  !WARNING: String edit descriptor in READ format expression
  read(*,'("abc")')

  !ERROR: Unterminated string
  !ERROR: Unterminated format expression
  read(*,'("abc)')

  !ERROR: 'H' edit descriptor in READ format expression
  read(*,'(3Habc)')

  !ERROR: 'H' edit descriptor in READ format expression
  !ERROR: Unterminated format expression
  read(*,'(5Habc)')

  !ERROR: 'I' edit descriptor 'w' value must be positive
  read(*,'(I0)')
end
