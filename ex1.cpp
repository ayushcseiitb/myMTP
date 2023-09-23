int add( int x, int y) {
  if( x > 0 ){
    x = x + y;
  }else{
    x = x + x;
  }
  return x + 3;
}
