package org.tspr3;
import java.util.Objects;

/** Public facade; strategy implementations and selection policy are internal. */
public final class Tspr3 {
    /** AUTO selects A/B conservatively; C remains explicitly available. */
    public enum Algorithm {
        /** Bounded-sample selection. */ AUTO,
        /** Ternas and full-value fusion. */ A,
        /** Ternas with compact frequencies. */ B,
        /** Compact ternas with a block-prefix index. */ C
    }
    /** API version. */
    public static final String VERSION="0.1.0";
    private Tspr3() { }
    /** Returns a sorted copy.
     * @param input original integers, preserved
     * @return newly allocated sorted integers
     */
    public static int[] sort(int[] input) { return sort(input,Algorithm.AUTO); }
    /** Returns a sorted copy with a selected strategy.
     * @param input original integers, preserved
     * @param algorithm AUTO or explicit A/B/C
     * @return newly allocated sorted integers
     */
    public static int[] sort(int[] input,Algorithm algorithm) {
        Objects.requireNonNull(input,"input");Objects.requireNonNull(algorithm,"algorithm");
        return Strategies.get(algorithm==Algorithm.AUTO?choose(input):algorithm).sort(input);
    }
    /** Produces compact output. AUTO means B; A is unsupported.
     * @param input original integers, preserved
     * @param algorithm B, C or AUTO
     * @return ordered values and their occurrence counts
     */
    public static Frequencies frequencies(int[] input,Algorithm algorithm) {
        Objects.requireNonNull(input,"input");Objects.requireNonNull(algorithm,"algorithm");
        OrderingStrategy strategy=Strategies.get(algorithm==Algorithm.AUTO?Algorithm.B:algorithm);
        if(!(strategy instanceof FrequencyStrategy)) throw new IllegalArgumentException("A does not produce frequencies");
        return ((FrequencyStrategy)strategy).frequencies(input);
    }
    /** Uses strategy B to count values.
     * @param input original integers
     * @return ordered values and counts
     */
    public static Frequencies frequencies(int[] input) { return frequencies(input,Algorithm.B); }
    /** Selects A/B using a bounded sample; does not sort the original data.
     * @param input original integers
     * @return selected algorithm
     */
    public static Algorithm choose(int[] input) { return SamplingSelector.choose(input); }
    /** Caller-owned parallel arrays representing compact output. */
    public static final class Frequencies {
        /** Ordered distinct values. */
        public final int[] values;
        /** Corresponding positive counts. */
        public final int[] counts;
        Frequencies(int[] values,int[] counts) { this.values=values;this.counts=counts; }
    }
}
