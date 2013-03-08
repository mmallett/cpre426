
public class Test {
	
	static int k = 3;
	
	public static void main(String args[]){
		
		int start = 1, end = 8;
		
		for(int i=0; i<k-1; i++){
			start = start * 10 + 1; //shift left, set lsd to 1
			end = end * 10 + 8; //shift left, set lsd to 8
		}
		
		for(int i=start; i<=end; i++){
			System.out.println(i + " " + is_valid(i));
		}
		
	}
	
	/*
	 * check if the k digit layout is valid for the n-queen problem
	 * invalid if two queens are in the same row, or are on a diagonal
	 * collision by column is eliminated by generation method
	 */
	static boolean is_valid(int layout){
		
		int digits[] = new int[k];
		
		//parse digits into an array
		for(int i=k-1; i>=0; i--){
			digits[i] = layout %10;
			layout /= 10;
		}
		
		for(int i=0; i<k; i++){
			
			if(digits[i] == 0 || digits[i] == 9) return false; //case of invalid digits
			
			if(i == k-1) continue; //avoid index out of bounds. collisions will have been hit by now
			
			/*
			 * digit collisions
			 * note there is no need to check backwards
			 * the leftmost member of the collision will be able to detect the collision
			 * and return false
			 */
			for(int j=i+1; j<k; j++){
				if(digits[i] == digits[j]) //row collision
					return false; 
				else if(digits[i] == digits[j] + (j - i)) //diagonal down collision
					return false;
				else if(digits[i] == digits[j] - (j - i)) //diagonal up collision
					return false;
			}
		}
		
		return true;
	}

}
