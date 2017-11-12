package com.mapswithme.maps;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class BankActivity extends AppCompatActivity {
    ListView mListOptions;
    BankOptionsAdapter mOptionsAdapter;

    static final int OPTION_TYPE_TRANSACTION = 1;
    static final int OPTION_TYPE_SHOW_MAP = 2;

    static class BankOption {
        int option_type;
        String name;
        int res_icon;

        public BankOption(int type, String name, int icon) {
            this.option_type = type;
            this.name = name;
            this.res_icon = icon;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bank);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        LayoutInflater inflater = LayoutInflater.from(this);
        mListOptions = (ListView) findViewById(R.id.bank_list_view_options);
        mListOptions.addHeaderView(inflater.inflate(R.layout.layout_bank_banner, null));

        mOptionsAdapter = new BankOptionsAdapter(this);
        mListOptions.setAdapter(mOptionsAdapter);
        mOptionsAdapter.add(new BankOption(OPTION_TYPE_TRANSACTION, "Buy Airtime", R.drawable.ic_buy_airtime));
        mOptionsAdapter.add(new BankOption(OPTION_TYPE_SHOW_MAP, "Locate Agents", R.drawable.ic_locations_lion));
        mOptionsAdapter.add(new BankOption(OPTION_TYPE_TRANSACTION, "Send Money", R.drawable.ic_send_money));
        mOptionsAdapter.add(new BankOption(OPTION_TYPE_TRANSACTION, "Other Option", R.drawable.ic_buy_airtime));
        mOptionsAdapter.add(new BankOption(OPTION_TYPE_TRANSACTION, "Something", R.drawable.ic_buy_airtime));
        mOptionsAdapter.add(new BankOption(OPTION_TYPE_SHOW_MAP, "GPS", R.drawable.ic_locations_lion));

        mListOptions.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                BankOption option = (BankOption) parent.getItemAtPosition(position);
                if (option.option_type == OPTION_TYPE_SHOW_MAP) {
                    Intent intent = new Intent(BankActivity.this, DownloadResourcesLegacyActivity.class);
                    startActivity(intent);
                    finish();
                }
            }
        });
    }

    public static class BankOptionsAdapter extends ArrayAdapter<BankOption> {
        public BankOptionsAdapter(@NonNull Context context) {
            super(context, 0);
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
            BankOption option = getItem(position);

            StaticOptionsViewHolder holder;
            if (convertView == null) {
                LayoutInflater inflater = LayoutInflater.from(getContext());
                convertView = inflater.inflate(R.layout.item_bank_option, parent, false);
                holder = new StaticOptionsViewHolder(convertView);
                convertView.setTag(holder);
            } else {
                holder = (StaticOptionsViewHolder) convertView.getTag();
            }

            holder.icon.setImageResource(option.res_icon);
            holder.name.setText(option.name);

            return convertView;
        }

        private static class StaticOptionsViewHolder {
            ImageView icon;
            TextView name;

            public StaticOptionsViewHolder(View view) {
                icon = (ImageView) view.findViewById(R.id.iv__option_icon);
                name = (TextView) view.findViewById(R.id.tv__option_name);
            }
        }
    }

}
