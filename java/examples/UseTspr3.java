import org.tspr3.Tspr3;
import java.util.Arrays;
public class UseTspr3 {
    public static void main(String[] args) {
        int[] input={9,8,9,15,12,13,8,8,9};
        System.out.println(Arrays.toString(Tspr3.sort(input)));
        Tspr3.Frequencies counts=Tspr3.frequencies(input);
        for(int i=0;i<counts.values.length;i++) System.out.print("("+counts.values[i]+","+counts.counts[i]+") ");
        System.out.println();
    }
}
