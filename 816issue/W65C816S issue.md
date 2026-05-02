# W65C816 Reset issue

開発中の「EMUZ80 PICO/PICO2 adapter card for MEZ65C_RAM」で<br>
[W65C186のリセットがかからない問題](reset_error.png)がありました。<br>
色々調べて、やっと原因が分かりました。<br>
W65C816S(5V)とPICO/PICO2を(3.3V)を直接繋いで制御していますが、<br>
W65C816SのDC特性では、Highレベルの規定が3.3V以上必要になるので
3.3V系を直接繋いでも正常に動作しないようです。<br>

DC特性でW65C816Sは、[Vih = VDD×0.8(min4.75V) ∴Vih = 3.8V](W65C816S_Vih.png)<br>
W65C02Sは、[Vih = VDD×0.7(min4.75V) ∴Vih = 3.32V](W65C02S_Vih.png)<br>
W65C02SはVihが3.32Vでギリギリ実力で動いてるようです。<br>
恐らく個体差が出ると思われます。<br>
<br>
Z80のVihは2V程度なので、3.3V系をダイレクトに繋いでも大丈夫なのでしょう。<br>

試しに電圧4.5Vにしたところ、[W65C816Sにリセットがかかり立ち上がりました。](Successful_startup.png)<br>
ただし、電圧下げているのでどこまで安定動作するか分かりません。<br>

対策としては入力信号のレベル変換ですが、Vih 2V程度のバッファIC<br>
[(74VHCT541AFT)を通したところ、リセットがかかり正常に立ち上がりました。](buffer_test.png)<br>
MEZ65C_RAM上のメモリメモリ（AS6C4008）とCPLD（ATF22V10C）とPICO/PICO2は<br>
直結しても特に問題はなさそうです。<br>
